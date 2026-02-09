#include "pch.h"
#include "AssetDatabase.h"

void AssetDatabase::AddListener(Listener l)
{
    std::lock_guard lk(_mtx);
    _listeners.push_back(std::move(l));
}

void AssetDatabase::Emit(const AssetEvent& e)
{
    std::vector<Listener> copy;
    {
        std::lock_guard lk(_mtx);
        copy = _listeners;
    }
    for (auto& fn : copy)
        fn(e);
}

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

void AssetDatabase::OnFileEvent(const FsEvent& e)
{
    // meta 파일 자체 이벤트는 AssetDB가 직접 처리할 필요 없음(원본 이벤트를 기준으로 meta를 관리)
    if (e.absPath.extension() == L".meta")
        return;

    if (e.action == FsAction::Removed)
        Remove(e.absPath, e);
    else
        Upsert(e.absPath, e);
}

void AssetDatabase::Upsert(const fs::path& absPath, const FsEvent& srcEvent)
{
    // 3단계 MetaStore에 맞춰: meta 생성/로드
    MetaFile meta = MetaStore::LoadOrCreate(absPath);

    {
        std::lock_guard lk(_mtx);
        _pathToGuid[absPath.wstring()] = meta.guid;
        _guidToPath[meta.guid] = absPath;
    }

    DBG->LogW(L"[AssetDB] Upsert: " + absPath.wstring());

    Emit(AssetEvent{ srcEvent, meta.guid });
}

void AssetDatabase::Remove(const fs::path& absPath, const FsEvent& srcEvent)
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

    Emit(AssetEvent{ srcEvent, had ? removedGuid : Guid() });
}

std::vector<fs::path> AssetDatabase::ListChildrenFiles(const fs::path& folderAbs) const
{
    std::vector<fs::path> out;
    std::lock_guard lk(_mtx);

    // 매우 단순: 현재 DB에 등록된 파일 중 parent_path가 folderAbs인 것만
    // (성능/정규화는 다음 단계에서 개선)
    for (const auto& [pathW, guid] : _pathToGuid)
    {
        fs::path p(pathW);
        if (p.parent_path() == folderAbs)
            out.push_back(p);
    }
    return out;
}
