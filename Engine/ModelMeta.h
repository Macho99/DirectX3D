#pragma once
#include "MetaFile.h"
class ModelMeta : public MetaFile
{
    using Super = MetaFile;
public:
    ModelMeta() : Super(ResourceType::Model) {}
    ~ModelMeta() {}

    virtual int GetImportVersion() const override { return 4; }
    virtual int GetSerializeVersion() const override { return 4; }
};

