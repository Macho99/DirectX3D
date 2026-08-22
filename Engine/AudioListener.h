#pragma once

#include "MonoBehaviour.h"

class AudioListener : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(AudioListener)
public:
    AudioListener() = default;
    ~AudioListener() override = default;

    void Start() override;
    void LateUpdate() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnDestroy() override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
    }

private:
    void ApplyTransform();

    Vec3 _previousPosition = Vec3::Zero;
    bool _hasPreviousPosition = false;
};
