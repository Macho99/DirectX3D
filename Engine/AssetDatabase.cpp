#include "pch.h"
#include "AssetDatabase.h"

bool AssetDatabase::TryGetGuidByPath(const fs::path& absPath, Guid& out) const
{
    std::lock_guard lk(_mtx);
    auto it = _pathToGuid.find(absPath.wstring());
    if (it == _pathToGuid.end()) return false;
    out = it->second;
    return true;
}

bool AssetDatabase::TryGetPathByGuid(const Guid& guid, fs::path& out) const
{
    std::lock_guard lk(_mtx);
    auto it = _guidToPath.find(guid);
    if (it == _guidToPath.end()) return false;
    out = it->second;
    return true;
}

void AssetDatabase::ReconcileAndBuildFromMeta(const fs::path& rootAbs)
{
    // 0) 존재 확인
    if (!fs::exists(rootAbs) || !fs::is_directory(rootAbs))
    {
        DBG->LogW(L"[AssetDB] Reconcile failed: invalid root: " + rootAbs.wstring());
        return;
    }

    // 1) 루트 전체 스캔: 원본/메타 목록 수집
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

    // 2) 원본에 meta 없으면 생성
    for (auto& src : sources)
    {
        fs::path metaAbs = MetaStore::MetaPathForSource(src);
        if (!fs::exists(metaAbs))
        {
            // meta 생성 (GUID 부여)
            MetaFile m = MetaStore::LoadOrCreate(src);
            DBG->LogW(L"[Meta] Created: " + metaAbs.wstring());
        }
    }

    // 3) 고아 meta 삭제 (meta만 있고 원본이 없으면 삭제)
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

    // 4) AssetDB 채우기: "원본 파일 목록"을 기준으로 meta를 로드해서 매핑 구축
    //    (meta 파일 목록을 기준으로 하면 깨진 meta / 임의 meta 파일 케이스에서 꼬일 수 있어서 비추천)
    {
        std::lock_guard lk(_mtx);
        _pathToGuid.clear();
        _guidToPath.clear();
    }

    for (auto& src : sources)
    {
        fs::path metaAbs = MetaStore::MetaPathForSource(src);

        auto loaded = MetaStore::TryLoad(metaAbs);
        if (!loaded.has_value() || !loaded->guid.IsValid())
        {
            // 깨진 meta면 재생성
            MetaFile m = MetaStore::LoadOrCreate(src);
            loaded = m;
            DBG->LogW(L"[Meta] Recreated invalid: " + metaAbs.wstring());
        }

        {
            std::lock_guard lk(_mtx);
            _pathToGuid[src.wstring()] = loaded->guid;
            _guidToPath[loaded->guid] = src;
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
        Rename(e.oldAbsPath, e.absPath);
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
    Guid renamedGuid;
    bool oldFound = false;
    {
        std::lock_guard lk(_mtx);
        auto it = _pathToGuid.find(oldAbsPath.wstring());
        if (it != _pathToGuid.end())
        {
            renamedGuid = it->second;
            _pathToGuid.erase(it);
            oldFound = true;
        }
    }

    if (!oldFound)
    {
        // old가 DB에 없었다면 new의 meta에서 읽어오거나 생성
        MetaFile meta = MetaStore::LoadOrCreate(newAbsPath);
        renamedGuid = meta.guid;
        DBG->LogErrorW(L"[AssetDB] Rename: old path not found, treating as Insert: " + newAbsPath.wstring());
    }

    // 3) 이제 맵 갱신(락 안)
    {
        std::lock_guard lk(_mtx);

        // newAbsPath가 이미 다른 guid에 매핑돼 있었다면 정리(충돌 방지)
        auto itNew = _pathToGuid.find(newAbsPath.wstring());
        if (itNew != _pathToGuid.end() && itNew->second != renamedGuid)
        {
            Guid prev = itNew->second;
            _guidToPath.erase(prev);
            DBG->LogErrorW(L"[AssetDB] Rename conflict: new path already mapped. Replacing: " + newAbsPath.wstring());
        }

        _pathToGuid[newAbsPath.wstring()] = renamedGuid;
        _guidToPath[renamedGuid] = newAbsPath;
    }

    DBG->LogW(L"[AssetDB] Rename: " + oldAbsPath.wstring() + L" -> " + newAbsPath.wstring());
}

void AssetDatabase::Insert(const fs::path& absPath)
{
    // 3단계 MetaStore에 맞춰: meta 생성/로드
    MetaFile meta = MetaStore::LoadOrCreate(absPath);

    {
        std::lock_guard lk(_mtx);
        _pathToGuid[absPath.wstring()] = meta.guid;
        _guidToPath[meta.guid] = absPath;
    }

    DBG->LogW(L"[AssetDB] Insert: " + absPath.wstring());
}

void AssetDatabase::Remove(const fs::path& absPath)
{
    Guid removedGuid; // 없을 수도 있음
    bool had = false;

    {
        std::lock_guard lk(_mtx);
        auto it = _pathToGuid.find(absPath.wstring());
        if (it != _pathToGuid.end())
        {
            removedGuid = it->second;
            _pathToGuid.erase(it);
            _guidToPath.erase(removedGuid);
            had = true;
        }
    }

    DBG->LogW(L"[AssetDB] Remove: " + absPath.wstring());
}
