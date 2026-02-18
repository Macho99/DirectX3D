#pragma once
#include "AssetId.h"
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
    virtual ~MetaFile() = default;

    AssetId GetAssetId() const { return _assetId; }
    virtual wstring GetResourcePath();
    bool TryImport();
    void ReImport();
    virtual void Import() {}

    fs::path GetAbsPath() const { return _absPath; }

protected:
    wstring GetArtifactPath();

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

    friend class MetaStore;
};