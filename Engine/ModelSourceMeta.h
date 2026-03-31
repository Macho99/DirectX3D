#pragma once
#include "SubAssetMetaFile.h"
class ModelSourceMeta : public SubAssetMetaFile
{
    using Super = SubAssetMetaFile;
public:
    ModelSourceMeta();
    ~ModelSourceMeta();

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
    virtual void OnMenu() override;
    virtual bool IsReadOnly() const override { return true; }

protected:
    void Import() override;

private:
    virtual int GetVersion() const override { return 7; }
};

