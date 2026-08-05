#pragma once
#include "MonoBehaviour.h"

class PlayerAnimEventHandler : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(PlayerAnimEventHandler)
public:
    virtual void OnAnimationEvent(const AnimationEvent& animationEvent) override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
    }
};

