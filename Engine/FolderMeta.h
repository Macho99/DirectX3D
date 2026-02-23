#pragma once
#include "MetaFile.h"

class FolderMeta : public MetaFile
{
    using Super = MetaFile;
public:
    FolderMeta();
    ~FolderMeta();

protected:
    virtual string GetIconKey() const override { return "FolderIcon"; }
};

