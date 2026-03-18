#pragma once
#include "SubAssetMetaFile.h"
class ModelSourceMeta : public SubAssetMetaFile
{
    using Super = SubAssetMetaFile;
public:
    ModelSourceMeta();
    ~ModelSourceMeta();

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;

protected:
    void Import() override;

private:
    virtual int GetVersion() const override { return 5; }
};

