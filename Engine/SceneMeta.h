#pragma once
#include "MetaFile.h"
class SceneMeta : public MetaFile
{
    using Super = MetaFile;
public:
    SceneMeta() : Super(ResourceType::Scene) {}
    ~SceneMeta() {}

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
};

