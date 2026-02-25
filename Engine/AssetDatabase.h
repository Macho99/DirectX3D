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
    struct ReservedAssetId
    {
        fs::path folderPath;
        wstring fileName;
        AssetId assetId;
    };

public:
    AssetDatabase();
    ~AssetDatabase();

    void ReconcileAndBuildFromMeta(const fs::path& rootAbs);

    // 메인 스레드에서 호출(디바운스 끝난 ready 이벤트)
    void OnFileEvent(const FsEvent& e);

    bool TryGetAssetIdByPath(const fs::path& absPath, OUT AssetId& out) const;
    bool TryGetPathByAssetId(const AssetId& guid, OUT fs::path& out) const;
    bool TryGetMetaByAssetId(const AssetId& guid, OUT MetaFile*& out) const;
    bool TryGetMetaByPath(const fs::path& absPath, OUT MetaFile*& out) const;
    bool SearchAssetIdByPath(const fs::path& searchFolder, const wstring& fileName, OUT AssetId& assetId) const;

    void ReserveAssetId(const fs::path& folderAbs, const wstring& fileName, AssetId assetId);
    bool TryGetReservedAssetId(const fs::path& absPath, OUT AssetId& assetId);

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

    vector<ReservedAssetId> _reservedAssetIds;
};
