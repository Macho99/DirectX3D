#pragma once
#include "MonoBehaviour.h"

class ThirdPersonCamMove : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(ThirdPersonCamMove)

public:
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    void SetTarget(TransformRef target) { _target = target; }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
            CEREAL_NVP(_target),
            CEREAL_NVP(_moveSpeed),
            CEREAL_NVP(_sprintSpeed),
            CEREAL_NVP(_acceleration),
            CEREAL_NVP(_deceleration),
            CEREAL_NVP(_turnSpeed),
            CEREAL_NVP(_mouseSpeed),
            CEREAL_NVP(_minPitch),
            CEREAL_NVP(_maxPitch)
        );
    }

private:
    void RotateAroundParent();
    void MoveTarget(float dt);
    Transform* GetRotationPivot();

private:
    TransformRef _target;

    float _moveSpeed = 5.f;
    float _sprintSpeed = 10.f;
    float _acceleration = 20.f;
    float _deceleration = 25.f;
    float _turnSpeed = 360.f;

    float _mouseSpeed = 0.1f;
    float _minPitch = -60.f;
    float _maxPitch = 75.f;

    Vec3 _velocity = Vec3::Zero;
    POINT _prevMousePos = {};
    float _pitch = 0.f;
    float _yaw = 0.f;
    bool _rotationInitialized = false;
};

