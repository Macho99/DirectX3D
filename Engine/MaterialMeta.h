#pragma once
#include "MetaFile.h"
class MaterialMeta : public MetaFile
{
    using Super = MetaFile;
public:
    MaterialMeta();
    ~MaterialMeta() {}

private:
    virtual int GetImportVersion() const override { return 2; }
    virtual int GetSerializeVersion() const override { return 2; }
};

