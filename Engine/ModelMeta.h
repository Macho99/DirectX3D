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

private:
    string RemapTextureNamesToAssetId(string materialPath);
};

