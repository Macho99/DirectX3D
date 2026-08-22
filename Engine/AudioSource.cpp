#include "pch.h"
#include "AudioSource.h"
#include "OnGUIUtils.h"
#include "Transform.h"

AudioSource::~AudioSource()
{
    ReleaseSound();
}

void AudioSource::Start()
{
    if (_playOnAwake)
        Play();
}

void AudioSource::Update()
{
    Super::Update();
    if (_soundInitialized && _spatialized)
        ApplyTransform();
}

void AudioSource::OnDisable()
{
    Pause();
}

void AudioSource::OnDestroy()
{
    ReleaseSound();
}

bool AudioSource::Play()
{
    return EnsureSound() && ma_sound_start(&_sound) == MA_SUCCESS;
}

void AudioSource::Pause()
{
    if (_soundInitialized)
        ma_sound_stop(&_sound);
}

void AudioSource::Stop()
{
    if (_soundInitialized)
    {
        ma_sound_stop(&_sound);
        ma_sound_seek_to_pcm_frame(&_sound, 0);
    }
}

bool AudioSource::IsPlaying() const
{
    return _soundInitialized && ma_sound_is_playing(&_sound) == MA_TRUE;
}

void AudioSource::SetClip(const AudioClipRef& clip)
{
    if (_clip != clip)
    {
        ReleaseSound();
        _clip = clip;
    }
}

void AudioSource::SetVolume(float volume)
{
    _volume = std::max(0.f, volume);
    if (_soundInitialized) ma_sound_set_volume(&_sound, _volume);
}

void AudioSource::SetPitch(float pitch)
{
    _pitch = std::max(0.01f, pitch);
    if (_soundInitialized) ma_sound_set_pitch(&_sound, _pitch);
}

void AudioSource::SetLooping(bool looping)
{
    _looping = looping;
    if (_soundInitialized) ma_sound_set_looping(&_sound, looping ? MA_TRUE : MA_FALSE);
}

void AudioSource::SetSpatialized(bool spatialized)
{
    _spatialized = spatialized;
    if (_soundInitialized) ma_sound_set_spatialization_enabled(&_sound, spatialized ? MA_TRUE : MA_FALSE);
}

void AudioSource::SetRandomClipAndPlay(const vector<AudioClipRef>& clips)
{
    if (clips.empty())
        return;

    int randomIndex = rand() % clips.size();
    if (clips[randomIndex].Resolve() == nullptr)
        return;

    SetClip(clips[randomIndex]);
    Play();
}

bool AudioSource::OnGUI()
{
    bool changed = Super::OnGUI();
    AudioClipRef clip = _clip;
    if (OnGUIUtils::DrawResourceRef("Audio Clip", clip))
    {
        SetClip(clip);
        changed = true;
    }

    const AudioGroup previousGroup = _group;
    const bool previousStreaming = _streaming;
    changed |= OnGUIUtils::DrawEnumCombo("Group", _group, AudioGroupNames, static_cast<int>(AudioGroup::Max));
    changed |= OnGUIUtils::DrawBool("Play On Awake", &_playOnAwake);
    changed |= OnGUIUtils::DrawBool("Loop", &_looping);
    changed |= OnGUIUtils::DrawBool("Stream", &_streaming);
    changed |= OnGUIUtils::DrawBool("Spatialized", &_spatialized);
    changed |= OnGUIUtils::DrawFloat("Volume", &_volume, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Pitch", &_pitch, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Pan", &_pan, 0.01f);
    changed |= OnGUIUtils::DrawEnumCombo("Attenuation", _attenuation, AudioAttenuationNames, static_cast<int>(AudioAttenuation::Max));
    changed |= OnGUIUtils::DrawFloat("Min Distance", &_minDistance, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Max Distance", &_maxDistance, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Doppler Factor", &_dopplerFactor, 0.01f);

    _volume = std::max(0.f, _volume);
    _pitch = std::max(0.01f, _pitch);
    _pan = std::clamp(_pan, -1.f, 1.f);
    _minDistance = std::max(0.f, _minDistance);
    _maxDistance = std::max(_minDistance, _maxDistance);
    _dopplerFactor = std::max(0.f, _dopplerFactor);

    if (_soundInitialized && (previousGroup != _group || previousStreaming != _streaming))
    {
        const bool wasPlaying = IsPlaying();
        ReleaseSound();
        if (wasPlaying) Play();
    }
    else if (_soundInitialized)
    {
        ApplySettings();
    }

    if (ImGui::Button(IsPlaying() ? "Pause" : "Play"))
    {
        if (IsPlaying()) Pause(); else Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) Stop();
    return changed;
}

bool AudioSource::EnsureSound()
{
    if (_soundInitialized)
        return true;

    SoundManager* soundManager = GET_SINGLE(SoundManager);
    ma_engine* engine = soundManager->GetEngine();
    AudioClip* clip = _clip.Resolve();
    if (engine == nullptr || clip == nullptr || clip->GetPath().empty())
        return false;

    ma_uint32 flags = _streaming ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
    if (!_spatialized) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

    const ma_result result = ma_sound_init_from_file_w(engine, clip->GetPath().c_str(), flags,
        soundManager->GetGroup(_group), nullptr, &_sound);
    if (result != MA_SUCCESS)
    {
        DBG->LogError("[AudioSource] Failed to load sound: " + string(ma_result_description(result)));
        return false;
    }

    _soundInitialized = true;
    _hasPreviousPosition = false;
    ApplySettings();
    ApplyTransform();
    return true;
}

void AudioSource::ReleaseSound()
{
    if (_soundInitialized)
    {
        ma_sound_uninit(&_sound);
        _sound = {};
        _soundInitialized = false;
        _hasPreviousPosition = false;
    }
}

void AudioSource::ApplySettings()
{
    if (!_soundInitialized) return;
    ma_sound_set_volume(&_sound, _volume);
    ma_sound_set_pitch(&_sound, _pitch);
    ma_sound_set_pan(&_sound, _pan);
    ma_sound_set_looping(&_sound, _looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_spatialization_enabled(&_sound, _spatialized ? MA_TRUE : MA_FALSE);
    ma_sound_set_attenuation_model(&_sound, ToMiniaudioAttenuation(_attenuation));
    ma_sound_set_min_distance(&_sound, _minDistance);
    ma_sound_set_max_distance(&_sound, _maxDistance);
    ma_sound_set_doppler_factor(&_sound, _dopplerFactor);
}

void AudioSource::ApplyTransform()
{
    Transform* transform = GetTransform();
    if (!_soundInitialized || !_spatialized || transform == nullptr) return;

    const Vec3 position = transform->GetPosition();
    const Vec3 forward = transform->GetLook();
    ma_sound_set_position(&_sound, position.x, position.y, -position.z);
    ma_sound_set_direction(&_sound, forward.x, forward.y, -forward.z);

    Vec3 velocity = Vec3::Zero;
    if (_hasPreviousPosition && DT > 0.f)
        velocity = (position - _previousPosition) / DT;
    ma_sound_set_velocity(&_sound, velocity.x, velocity.y, -velocity.z);

    _previousPosition = position;
    _hasPreviousPosition = true;
}

ma_attenuation_model AudioSource::ToMiniaudioAttenuation(AudioAttenuation attenuation)
{
    switch (attenuation)
    {
    case AudioAttenuation::None: return ma_attenuation_model_none;
    case AudioAttenuation::Linear: return ma_attenuation_model_linear;
    case AudioAttenuation::Exponential: return ma_attenuation_model_exponential;
    default: return ma_attenuation_model_inverse;
    }
}
