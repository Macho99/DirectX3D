#pragma once

#include "ResourceBase.h"
#include "ResourceRef.h"

class AudioClip : public ResourceBase
{
    using Super = ResourceBase;
public:
    static constexpr ResourceType StaticType = ResourceType::AudioClip;

    AudioClip();
    ~AudioClip() override = default;

    void Load(const wstring& path) override;
    const wstring& GetPath() const { return _path; }
};

using AudioClipRef = ResourceRef<AudioClip>;
