#pragma once
#include "MetaFile.h"
class MaterialMeta : public MetaFile
{
    using Super = MetaFile;
public:
    MaterialMeta();
    ~MaterialMeta() {}

private:
    virtual int GetVersion() const override { return 2; }
};

