#pragma once

#include "MetaStore.h"
#include "DirectoryWatcherWin.h"

struct AssetEvent
{
    FsEvent fs;
    Guid guid;
};

class AssetDatabase
{
public:
    AssetDatabase() {}
    ~AssetDatabase() {}

    void ReconcileAndBuildFromMeta(const fs::path& rootAbs);

    // 메인 스레드에서 호출(디바운스 끝난 ready 이벤트)
    void OnFileEvent(const FsEvent& e);

    bool TryGetGuidByPath(const fs::path& absPath, Guid& out) const;
    bool TryGetPathByGuid(const Guid& guid, fs::path& out) const;

private:
    void Rename(const fs::path& oldAbsPath, const fs::path& newAbsPath);
    void Insert(const fs::path& absPath);
    void Remove(const fs::path& absPath);

private:
    mutable mutex _mtx;

    // 경로는 key로 wstring이 편합니다(대소문자/정규화는 다음 단계에서)
    unordered_map<wstring, Guid> _pathToGuid;
    unordered_map<Guid, fs::path, GuidHash> _guidToPath;
};
