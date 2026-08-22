#pragma once

#include "MetaFile.h"

class AudioMeta : public MetaFile
{
    using Super = MetaFile;
public:
    AudioMeta() : Super(ResourceType::AudioClip) {}
    ~AudioMeta() override = default;
};
