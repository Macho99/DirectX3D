#pragma once

#include "MetaFile.h"

class ModelMeshMeta : public MetaFile
{
    using Super = MetaFile;

public:
    ModelMeshMeta();
    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
};
