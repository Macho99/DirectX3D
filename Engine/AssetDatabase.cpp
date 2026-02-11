#include "pch.h"
#include "MetaStore.h"
#include "AssetDatabase.h"

AssetDatabase::AssetDatabase()
{
}

AssetDatabase::~AssetDatabase()
{
}

bool AssetDatabase::TryGetAssetIdByPath(const fs::path& absPath, OUT AssetId& out) const
{
    std::lock_guard lk(_mtx);
    auto it = _pathToAssetId.find(absPath.wstring());
    if (it == _pathToAssetId.end()) return false;
    out = it->second;
    return true;
}

bool AssetDatabase::TryGetPathByAssetId(const AssetId& assetId, OUT fs::path& out) const
{
    std::lock_guard lk(_mtx);
    auto it = _assetIdToMeta.find(assetId);
    if (it == _assetIdToMeta.end()) 
        return false;
    out = it->second->GetAbsPath();
    return true;
}

void AssetDatabase::ReconcileAndBuildFromMeta(const fs::path& rootAbs)
{
    // 존재 확인
    if (!fs::exists(rootAbs) || !fs::is_directory(rootAbs))
    {
        DBG->LogW(L"[AssetDB] Reconcile failed: invalid root: " + rootAbs.wstring());
        return;
    }

    // 루트 전체 스캔: 원본/메타 목록 수집
    std::vector<fs::path> sources;
    std::vector<fs::path> metas;

    try
    {
        for (auto& de : fs::recursive_directory_iterator(rootAbs))
        {
            fs::path p = de.path();

            if (MetaStore::IsMetaFile(p))
            {
                metas.push_back(p);
                continue;
            }

            sources.push_back(p);
        }
    }
    catch (...)
    {
        DBG->LogW(L"[AssetDB] Reconcile: directory iteration exception");
        return;
    }

    // 고아 meta 삭제 (meta만 있고 원본이 없으면 삭제)
    for (auto& metaAbs : metas)
    {
        fs::path src = MetaStore::SourcePathForMeta(metaAbs);

        // 원본이 없으면 meta 삭제
        if (!fs::exists(src))
        {
            std::error_code ec;
            fs::remove(metaAbs, ec);
            if (!ec)
                DBG->LogW(L"[Meta] Deleted orphan: " + metaAbs.wstring());
            else
                DBG->LogW(L"[Meta] Failed to delete orphan: " + metaAbs.wstring());
        }
    }

    // AssetDB 채우기: "원본 파일 목록"을 기준으로 meta를 로드해서 매핑 구축
    // (meta 파일 목록을 기준으로 하면 깨진 meta / 임의 meta 파일 케이스에서 꼬일 수 있어서 비추천)
    {
        std::lock_guard lk(_mtx);
        _pathToAssetId.clear();
        _assetIdToMeta.clear();
    }

    for (auto& src : sources)
    {
        fs::path metaAbs = MetaStore::MetaPathForSource(src);

        unique_ptr<MetaFile> loaded = MetaStore::TryLoad(metaAbs);
        if (loaded == nullptr || !loaded->GetAssetId().IsValid())
        {
            // 깨진 meta면 재생성
            loaded = MetaStore::Create(src);
            DBG->LogW(L"[Meta] Recreated invalid: " + metaAbs.wstring());
        }
        {
            std::lock_guard lk(_mtx);
            _pathToAssetId[src.wstring()] = loaded->GetAssetId();
            _assetIdToMeta[loaded->GetAssetId()] = std::move(loaded);
        }
    }

    DBG->LogW(L"[AssetDB] Reconcile+Build done. sources=" + std::to_wstring(sources.size())
        + L", metas=" + std::to_wstring(metas.size()));
}

void AssetDatabase::OnFileEvent(const FsEvent& e)
{
    if (MetaStore::IsMetaFile(e.absPath))
        return;

    if (e.action == FsAction::Removed)
        Remove(e.absPath);
    else if (e.action == FsAction::Renamed)
    {
        if (e.oldAbsPath.extension() != e.absPath.extension())
            ExtensionRename(e.oldAbsPath, e.absPath);
        else
            Rename(e.oldAbsPath, e.absPath);
    }
    else
        Insert(e.absPath);
}

void AssetDatabase::Rename(const fs::path& oldAbsPath, const fs::path& newAbsPath)
{
    // 1) meta 파일도 같이 이동(있으면)
    const fs::path oldMeta = MetaStore::MetaPathForSource(oldAbsPath);
    const fs::path newMeta = MetaStore::MetaPathForSource(newAbsPath);

    // meta 이동은 파일 시스템 상태에 따라 실패할 수 있으니 에러코드로 조용히 처리
    if (fs::exists(oldMeta))
    {
        std::error_code ec;
        fs::rename(oldMeta, newMeta, ec);
        if (ec)
        {
            // rename 실패 시: fallback(복사 후 삭제)
            std::error_code ec2;
            fs::copy_file(oldMeta, newMeta, fs::copy_options::overwrite_existing, ec2);
            if (!ec2) fs::remove(oldMeta, ec2);

            DBG->LogErrorW(L"[AssetDB] Rename meta fallback: " + oldMeta.wstring() + L" -> " + newMeta.wstring());
        }
    }

    // 2) guid 결정은 락 밖에서 (필요하면 meta에서 읽기/생성)
    AssetId renamedAssetId;
    bool oldFound = false;
    {
        std::lock_guard lk(_mtx);
        auto it = _pathToAssetId.find(oldAbsPath.wstring());
        if (it != _pathToAssetId.end())
        {
            renamedAssetId = it->second;
            _pathToAssetId.erase(it);
            oldFound = true;
        }
    }

    if (!oldFound)
    {
        // old가 DB에 없었다면 new의 meta에서 읽어오거나 생성
        unique_ptr<MetaFile> meta = MetaStore::Create(newAbsPath);
        renamedAssetId = meta->GetAssetId();
        {
            std::lock_guard lk(_mtx);
            _assetIdToMeta[renamedAssetId] = std::move(meta);
        }
        DBG->LogErrorW(L"[AssetDB] Rename: old path not found, treating as Insert: " + newAbsPath.wstring());
    }

    // 3) 이제 맵 갱신(락 안)
    {
        std::lock_guard lk(_mtx);

        // newAbsPath가 이미 다른 guid에 매핑돼 있었다면 정리(충돌 방지)
        auto itNew = _pathToAssetId.find(newAbsPath.wstring());
        if (itNew != _pathToAssetId.end() && itNew->second != renamedAssetId)
        {
            AssetId prev = itNew->second;
            _assetIdToMeta.erase(prev);
            DBG->LogErrorW(L"[AssetDB] Rename conflict: new path already mapped. Replacing: " + newAbsPath.wstring());
        }

        _pathToAssetId[newAbsPath.wstring()] = renamedAssetId;
    }

    DBG->LogW(L"[AssetDB] Rename: " + oldAbsPath.wstring() + L" -> " + newAbsPath.wstring());
}

void AssetDatabase::ExtensionRename(const fs::path& oldAbsPath, const fs::path& newAbsPath)
{
    // 확장자 변경은 완전 삭제 후 삽입으로 처리
    DBG->LogW(L"[AssetDB] ExtensionRename: " + oldAbsPath.wstring() + L" -> " + newAbsPath.wstring());
    Remove(oldAbsPath);
    Insert(newAbsPath);
}

void AssetDatabase::Insert(const fs::path& absPath)
{
    unique_ptr<MetaFile> meta = MetaStore::Create(absPath);

    {
        AssetId assetId = meta->GetAssetId();
        std::lock_guard lk(_mtx);
        _pathToAssetId[absPath.wstring()] = assetId;
        _assetIdToMeta[assetId] = std::move(meta);
    }

    DBG->LogW(L"[AssetDB] Insert: " + absPath.wstring());
}

void AssetDatabase::Remove(const fs::path& absPath)
{
    {
        // 1) meta 파일도 같이 삭제(있으면)
        const fs::path metaAbs = MetaStore::MetaPathForSource(absPath);
        std::error_code ec;
        fs::remove(metaAbs, ec);
        if (!ec)
            DBG->LogW(L"[Meta] Deleted on Remove: " + metaAbs.wstring());
    }
    // 2) 맵에서 제거
    {
        std::lock_guard lk(_mtx);
        auto it = _pathToAssetId.find(absPath.wstring());
        if (it != _pathToAssetId.end())
        {
            AssetId removedAssetId = it->second;
            _pathToAssetId.erase(it);
            _assetIdToMeta.erase(removedAssetId);
        }
    }

    DBG->LogW(L"[AssetDB] Remove: " + absPath.wstring());
}
