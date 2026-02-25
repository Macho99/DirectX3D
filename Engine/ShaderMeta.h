#pragma once
#include "MetaFile.h"
class ShaderMeta : public MetaFile
{
    using Super = MetaFile;
public:
    ShaderMeta();
    ~ShaderMeta();

protected:
    unique_ptr<ResourceBase> LoadResource() const override;
};

