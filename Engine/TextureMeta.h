#pragma once
#include "MetaFile.h"
class TextureMeta : public MetaFile
{
public:
    TextureMeta() : MetaFile(ResourceType::Texture) {}
    ~TextureMeta() {}

protected:
    virtual unique_ptr<ResourceBase> LoadResource() const override;
};

