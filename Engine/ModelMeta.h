#pragma once
#include "SubAssetMetaFile.h"
class ModelMeta : public SubAssetMetaFile
{
    using Super = SubAssetMetaFile;
public:
    ModelMeta();
    ~ModelMeta();

    void Import() override;
};

