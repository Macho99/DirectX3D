#include "pch.h"
#include "AudioClip.h"

AudioClip::AudioClip() : Super(StaticType)
{
}

void AudioClip::Load(const wstring& path)
{
    Super::Load(path);
}
