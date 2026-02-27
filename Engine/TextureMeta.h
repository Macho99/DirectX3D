#pragma once
#include "MetaFile.h"
class TextureMeta : public MetaFile
{
public:
    TextureMeta() : MetaFile(ResourceType::Texture) {}
    ~TextureMeta() {}
};

