#pragma once
#include "MetaFile.h"

class AnimationMeta : public MetaFile
{
    using Super = MetaFile;
public:
    AnimationMeta(): Super(ResourceType::Animation) {}
    ~AnimationMeta() {}
};