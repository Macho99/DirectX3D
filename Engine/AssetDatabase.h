#pragma once

#include "AssetId.h"
#include "DirectoryWatcherWin.h"

struct AssetEvent
{
    FsEvent fs;
    AssetId assetId;
};

class AssetDatabase
{
public:
    AssetDatabase() {}
    ~AssetDatabase() {}

    void ReconcileAndBuildFromMeta(const fs::path& rootAbs);

    // 메인 스레드에서 호출(디바운스 끝난 ready 이벤트)
    void OnFileEvent(const FsEvent& e);

    bool TryGetGuidByPath(const fs::path& absPath, AssetId& out) const;
    bool TryGetPathByGuid(const AssetId& guid, fs::path& out) const;

private:
    void Rename(const fs::path& oldAbsPath, const fs::path& newAbsPath);
    void Insert(const fs::path& absPath);
    void Remove(const fs::path& absPath);

private:
    mutable mutex _mtx;

    // 경로는 key로 wstring이 편합니다(대소문자/정규화는 다음 단계에서)
    unordered_map<wstring, AssetId> _pathToGuid;
    unordered_map<AssetId, fs::path, AssetIdHash> _guidToPath;
};
