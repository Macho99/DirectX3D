#pragma once
#include "MetaFile.h"
class NotSupportMeta : public MetaFile
{
    using Super = MetaFile;
public:
    NotSupportMeta() : Super(ResourceType::None) {}
    ~NotSupportMeta() = default;

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override
    {
        assert(false && "NotSupportMeta::LoadResource: This resource type is not supported.");
        return nullptr;
    }
};

