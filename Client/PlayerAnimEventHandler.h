#pragma once
#include "MonoBehaviour.h"

class AudioClip;

class PlayerAnimEventHandler : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(PlayerAnimEventHandler)

public:
    virtual void Start() override;
    virtual void Update() override;
    virtual void OnAnimationEvent(const AnimationEvent& animationEvent) override;
    virtual int GetVersion() const override { return 2; }
    virtual bool OnGUI() override;

private:
    void UpdateTrailRenderer(bool force = false);

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_trailRenderer));

        if (_version >= 1)
        {
            ar(CEREAL_NVP(_swordAudioClips));
        }

        if (_version >= 2)
        {
            ar(CEREAL_NVP(_impulseAudioClips));
        }
    }

private:
    vector<ResourceRef<AudioClip>> _swordAudioClips;
    vector<ResourceRef<AudioClip>> _impulseAudioClips;

    ComponentRef<class AudioSource> _audioSource;
    ComponentRef<class TrailRenderer> _trailRenderer;
    float _trailUpdateTime = FLT_MAX;
    int32 _trailAnimationIndex = -1;
};

