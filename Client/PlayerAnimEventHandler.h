#pragma once
#include "MonoBehaviour.h"

class PlayerAnimEventHandler : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(PlayerAnimEventHandler)

public:
    virtual void Start() override;
    virtual void Update() override;
    virtual void OnAnimationEvent(const AnimationEvent& animationEvent) override;

private:
    void UpdateTrailRenderer(bool force = false);

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_trailRenderer));
    }

private:
    ComponentRef<class TrailRenderer> _trailRenderer;
    float _trailUpdateTime = FLT_MAX;
    int32 _trailAnimationIndex = -1;
};

