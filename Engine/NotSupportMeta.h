#pragma once
#include "MetaFile.h"
class NotSupportMeta : public MetaFile
{
    using Super = MetaFile;
public:
    NotSupportMeta() : Super(ResourceType::None) {}
    ~NotSupportMeta() = default;
};

