#pragma once

#include "miniaudio.h"

enum class AudioGroup : uint8
{
    SFX,
    BGM,
    UI,
    Voice,
    Max
};

inline constexpr const char* AudioGroupNames[] = { "SFX", "BGM", "UI", "Voice" };

class SoundManager
{
    DECLARE_SINGLE_WITH_CONSTRUCTOR(SoundManager);
    ~SoundManager();

public:
    bool Init();
    void OnDestroy();

    bool IsInitialized() const { return _initialized; }
    ma_engine* GetEngine();
    ma_sound_group* GetGroup(AudioGroup group);

    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return _masterVolume; }
    void SetGroupVolume(AudioGroup group, float volume);
    float GetGroupVolume(AudioGroup group) const;

private:
    static constexpr size_t GROUP_COUNT = static_cast<size_t>(AudioGroup::Max);

    ma_engine _engine{};
    ma_sound_group _groups[GROUP_COUNT]{};
    bool _groupInitialized[GROUP_COUNT]{};
    bool _initialized = false;
    float _masterVolume = 1.f;
    float _groupVolumes[GROUP_COUNT] = { 1.f, 1.f, 1.f, 1.f };
};
