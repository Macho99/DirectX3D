#pragma once
#include "MetaFile.h"

class FolderMeta : public MetaFile
{
    using Super = MetaFile;
public:
    FolderMeta();
    ~FolderMeta();
};

