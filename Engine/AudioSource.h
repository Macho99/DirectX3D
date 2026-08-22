#pragma once

#include "MonoBehaviour.h"
#include "AudioClip.h"
#include "SoundManager.h"

enum class AudioAttenuation : uint8
{
    None,
    Inverse,
    Linear,
    Exponential,
    Max
};

inline constexpr const char* AudioAttenuationNames[] = { "None", "Inverse", "Linear", "Exponential" };

class AudioSource : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(AudioSource)
public:
    AudioSource() = default;
    ~AudioSource() override;

    void Start() override;
    void Update() override;
    void OnDisable() override;
    void OnDestroy() override;
    bool OnGUI() override;

    bool Play();
    void Pause();
    void Stop();
    bool IsPlaying() const;

    void SetClip(const AudioClipRef& clip);
    const AudioClipRef& GetClip() const { return _clip; }
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetLooping(bool looping);
    void SetSpatialized(bool spatialized);

    void SetRandomClipAndPlay(const vector<AudioClipRef>& clips);

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_clip), CEREAL_NVP(_group), CEREAL_NVP(_playOnAwake),
            CEREAL_NVP(_looping), CEREAL_NVP(_streaming), CEREAL_NVP(_spatialized),
            CEREAL_NVP(_volume), CEREAL_NVP(_pitch), CEREAL_NVP(_pan),
            CEREAL_NVP(_attenuation), CEREAL_NVP(_minDistance),
            CEREAL_NVP(_maxDistance), CEREAL_NVP(_dopplerFactor));
    }

private:
    bool EnsureSound();
    void ReleaseSound();
    void ApplySettings();
    void ApplyTransform();
    static ma_attenuation_model ToMiniaudioAttenuation(AudioAttenuation attenuation);

    AudioClipRef _clip;
    AudioGroup _group = AudioGroup::SFX;
    bool _playOnAwake = true;
    bool _looping = false;
    bool _streaming = false;
    bool _spatialized = false;
    float _volume = 1.f;
    float _pitch = 1.f;
    float _pan = 0.f;
    AudioAttenuation _attenuation = AudioAttenuation::Inverse;
    float _minDistance = 1.f;
    float _maxDistance = 100.f;
    float _dopplerFactor = 1.f;

    ma_sound _sound{};
    bool _soundInitialized = false;
    Vec3 _previousPosition = Vec3::Zero;
    bool _hasPreviousPosition = false;
};
