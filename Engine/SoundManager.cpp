#include "pch.h"
#include "SoundManager.h"

SoundManager::SoundManager()
{
}

SoundManager::~SoundManager()
{
    OnDestroy();
}

bool SoundManager::Init()
{
    if (_initialized)
        return true;

    const ma_result engineResult = ma_engine_init(nullptr, &_engine);
    if (engineResult != MA_SUCCESS)
    {
        DBG->LogError("[SoundManager] Failed to initialize miniaudio: " + string(ma_result_description(engineResult)));
        return false;
    }

    _initialized = true;
    for (size_t i = 0; i < GROUP_COUNT; ++i)
    {
        const ma_result result = ma_sound_group_init(&_engine, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &_groups[i]);
        if (result != MA_SUCCESS)
        {
            DBG->LogError("[SoundManager] Failed to initialize sound group: " + string(ma_result_description(result)));
            OnDestroy();
            return false;
        }
        _groupInitialized[i] = true;
        ma_sound_group_set_volume(&_groups[i], _groupVolumes[i]);
    }

    ma_engine_set_volume(&_engine, _masterVolume);
    return true;
}

void SoundManager::OnDestroy()
{
    if (!_initialized)
        return;

    for (size_t i = GROUP_COUNT; i > 0; --i)
    {
        const size_t index = i - 1;
        if (_groupInitialized[index])
        {
            ma_sound_group_uninit(&_groups[index]);
            _groupInitialized[index] = false;
        }
    }

    ma_engine_uninit(&_engine);
    _initialized = false;
}

ma_engine* SoundManager::GetEngine()
{
    if (!_initialized && !Init())
        return nullptr;
    return &_engine;
}

ma_sound_group* SoundManager::GetGroup(AudioGroup group)
{
    if (!_initialized && !Init())
        return nullptr;

    const size_t index = static_cast<size_t>(group);
    return index < GROUP_COUNT && _groupInitialized[index] ? &_groups[index] : nullptr;
}

void SoundManager::SetMasterVolume(float volume)
{
    _masterVolume = std::max(0.f, volume);
    if (_initialized)
        ma_engine_set_volume(&_engine, _masterVolume);
}

void SoundManager::SetGroupVolume(AudioGroup group, float volume)
{
    const size_t index = static_cast<size_t>(group);
    if (index >= GROUP_COUNT)
        return;

    _groupVolumes[index] = std::max(0.f, volume);
    if (_initialized && _groupInitialized[index])
        ma_sound_group_set_volume(&_groups[index], _groupVolumes[index]);
}

float SoundManager::GetGroupVolume(AudioGroup group) const
{
    const size_t index = static_cast<size_t>(group);
    return index < GROUP_COUNT ? _groupVolumes[index] : 0.f;
}
