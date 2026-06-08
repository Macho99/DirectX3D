#pragma once
#include "MetaFile.h"
class FontMeta : public MetaFile
{
    using Super = MetaFile;
public:
    FontMeta() : Super(ResourceType::Font) {}
    ~FontMeta() {}
};

