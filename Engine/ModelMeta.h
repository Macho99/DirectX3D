#pragma once
#include "SubAssetMetaFile.h"
class ModelMeta : public SubAssetMetaFile
{
    using Super = SubAssetMetaFile;
public:
    ModelMeta();
    ~ModelMeta();

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;

protected:
    void Import() override;
};

