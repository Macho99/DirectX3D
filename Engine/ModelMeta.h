#pragma once
#include "SubAssetMetaFile.h"
class ModelMeta : public SubAssetMetaFile
{
    using Super = SubAssetMetaFile;
public:
    ModelMeta();
    ~ModelMeta();

protected:
    void Import() override;
    virtual string GetIconKey() const override { return "ModelIcon"; }

private:
    string RemapTextureNamesToAssetId(string materialPath);
};

