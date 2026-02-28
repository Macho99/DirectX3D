#pragma once
#include "AssetId.h"
#include "ImportManifest.h"
#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/string.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/polymorphic.hpp"

class MetaFile
{
public:
    MetaFile();
    MetaFile(ResourceType resourceType);
    virtual ~MetaFile();

    virtual void OnLoad(unordered_map<AssetId, MetaFile*, AssetIdHash>& subAssetContainer) {}
    virtual void OnDestroy(unordered_map<AssetId, MetaFile*, AssetIdHash>& subAssetContainer) {}
    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const;

    Texture* GetIconTexture() const;
    virtual void DrawContentBrowserItem(fs::path& _selectedPath, fs::path& _currentFolder, float _thumbSize, int& curCol, int columns) const;

public:
    AssetId GetAssetId() const { return _assetId; }
    virtual wstring GetResourcePath() const;

    fs::path GetAbsPath() const { return _absPath; }
    void SetAbsPath(const fs::path& absPath) { _absPath = absPath; }
    ResourceType GetResourceType() const { return _resourceType; }

protected:
    virtual void Import();
    wstring GetArtifactPath() const;

    Texture* GetIconTexture(ResourceType resourceType, const AssetId& assetId, const fs::path& absPath) const;
    unique_ptr<ResourceBase> LoadResource(ResourceType resourceType, const fs::path& filePath) const;

private:
    void ForceReImport();
    bool ImportIfDirty();
    wstring GetManifestPath() { return GetArtifactPath() + L"\\asset.manifest"; }

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(_assetId));
        ar(CEREAL_NVP(_resourceType));
    }
    
protected:
    AssetId _assetId;
    ResourceType _resourceType;

    fs::path _absPath;
private:
    ImportManifest _importManifest;
    
    friend class MetaStore;
};