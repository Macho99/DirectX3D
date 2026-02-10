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
    MetaFile(ResourceType resourceType);
    virtual ~MetaFile() = default;

    AssetId GetAssetId() const { return assetId; }
    virtual wstring GetResourcePath() = 0;

protected:
    wstring GetArtifactPath();

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(assetId));
        ar(CEREAL_NVP(_resourceType));
    }

private:
    AssetId assetId;
    ResourceType _resourceType;
};

