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

    AssetId GetAssetId() const { return _assetId; }
    virtual wstring GetResourcePath();
    void ImportIfDirty();
    void ForceReImport();

    fs::path GetAbsPath() const { return _absPath; }

protected:
    virtual void Import();
    wstring GetArtifactPath();

private:
    wstring GetManifestPath() { return GetArtifactPath() + L"\\asset.manifest"; }

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(_assetId));
        ar(CEREAL_NVP(_resourceType));
    }

private:
    AssetId _assetId;
    ResourceType _resourceType;

    fs::path _absPath;
    ImportManifest _importManifest;
    
    friend class MetaStore;
};