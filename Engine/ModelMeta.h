#pragma once
#include "MetaFile.h"
class ModelMeta : public MetaFile
{
    using Super = MetaFile;
public:
    ModelMeta() : Super(ResourceType::Model) {}
    ~ModelMeta() {}

    virtual int GetVersion() const override { return 2; }
};

