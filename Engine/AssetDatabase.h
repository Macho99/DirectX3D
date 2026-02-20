#pragma once

#include "AssetId.h"
#include "DirectoryWatcherWin.h"

class MetaFile;

struct AssetEvent
{
    FsEvent fs;
    AssetId assetId;
};

class AssetDatabase
{
public:
    AssetDatabase();
    ~AssetDatabase();

    void ReconcileAndBuildFromMeta(const fs::path& rootAbs);

    // 메인 스레드에서 호출(디바운스 끝난 ready 이벤트)
    void OnFileEvent(const FsEvent& e);

    bool TryGetAssetIdByPath(const fs::path& absPath, OUT AssetId& out) const;
    bool TryGetPathByAssetId(const AssetId& guid, OUT fs::path& out) const;

private:
    void Rename(const fs::path& oldAbsPath, const fs::path& newAbsPath);
    void ExtensionRename(const fs::path& oldAbsPath, const fs::path& newAbsPath);
    void Insert(const fs::path& absPath);
    void Remove(const fs::path& absPath);
    void Modify(const fs::path& absPath);

private:
    mutable mutex _mtx;

    unordered_map<wstring, AssetId> _pathToAssetId;
    unordered_map<AssetId, unique_ptr<MetaFile>, AssetIdHash> _assetIdToMeta;
};
