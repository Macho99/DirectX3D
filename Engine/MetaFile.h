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
    virtual string GetName(const AssetId& assetId);

    virtual Texture* GetIconTexture() const;
    virtual void DrawContentBrowserItem(fs::path& _currentFolder, float _thumbSize, int& curCol, int columns) const;

public:
    AssetId GetAssetId() const { return _assetId; }

    fs::path GetAssetPath() const { return _assetPath; }
    fs::path GetMetaPath() const { return GetAssetPath().wstring() + L".meta"; }
    fs::path GetImportedAssetPath() const;
    virtual fs::path GetImportedAssetPath(const AssetId& assetId) const;
    void SetAssetPath(const fs::path& assetPath) { _assetPath = assetPath; }
    ResourceType GetResourceType() const { return _resourceType; }

    virtual bool OnGUI();
    virtual void OnMenu();
    virtual bool IsReadOnly() const { return false; }

protected:
    virtual void Import();
    void ForceReimport();
    wstring GetArtifactPath() const;

    Texture* GetIconTexture(ResourceType resourceType, const AssetId& assetId, const fs::path& absPath) const;
    unique_ptr<ResourceBase> LoadResource(ResourceType resourceType, const fs::path& filePath) const;

private:
    bool ImportIfDirty();
    wstring GetManifestPath() { return GetArtifactPath() + L"\\asset.manifest"; }
    // Import version invalidates generated artifacts through ImportManifest.
    virtual int GetImportVersion() const { return 1; }
    // Serialize version only describes the .meta file schema.
    virtual int GetSerializeVersion() const { return 1; }

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        if (Archive::is_saving::value)
            _version = GetSerializeVersion();

        ar(CEREAL_NVP(_version));
        ar(CEREAL_NVP(_assetId));
        ar(CEREAL_NVP(_resourceType));
    }
    
protected:
    int _version = 1;
    AssetId _assetId;
    ResourceType _resourceType;

    fs::path _assetPath;
private:
    ImportManifest _importManifest;
    
    friend class MetaStore;
};
