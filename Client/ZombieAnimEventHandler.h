#pragma once
#include <MonoBehaviour.h>
class ZombieAnimEventHandler : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(ZombieAnimEventHandler)

public:
    virtual void Start() override;
    virtual void Update() override;
    virtual void OnAnimationEvent(const AnimationEvent& animationEvent) override;
    virtual int GetVersion() const override { return 1; }
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
            ar(CEREAL_NVP(_screamAudioClips));
            ar(CEREAL_NVP(_attackAudioClips));
        }
    }

private:
    vector<ResourceRef<AudioClip>> _screamAudioClips;
    vector<ResourceRef<AudioClip>> _attackAudioClips;

    ComponentRef<class AudioSource> _audioSource;
    ComponentRef<class TrailRenderer> _trailRenderer;
    ComponentRef<class MeshRenderer> _screamRenderer;
    float _trailUpdateTime = FLT_MAX;
    int32 _trailAnimationIndex = -1;
};

