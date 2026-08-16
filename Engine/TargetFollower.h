#pragma once
#include "Component.h"
#include "ComponentRef.h"

enum class TargetPositionFollowMode : uint8
{
    Immediate,
    Interpolated,
    Max
};

class TargetFollower : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(TargetFollower)

public:
    TargetFollower();
    ~TargetFollower();

    virtual void Update() override;
    virtual bool OnGUI() override;

    void SetTarget(Transform* target) { _target = TransformRef(target); }
    Transform* GetTarget() const { return _target.Resolve(); }

    void SetPositionOffset(const Vec3& offset) { _positionOffset = offset; }
    Vec3 GetPositionOffset() const { return _positionOffset; }

    void SetPositionFollowMode(TargetPositionFollowMode mode) { _positionFollowMode = mode; }
    TargetPositionFollowMode GetPositionFollowMode() const { return _positionFollowMode; }

    void SetPositionInterpolationSpeed(float speed) { _positionInterpolationSpeed = speed < 0.f ? 0.f : speed; }
    float GetPositionInterpolationSpeed() const { return _positionInterpolationSpeed; }

    void SetFollowPositionX(bool follow) { _followPositionX = follow; }
    void SetFollowPositionY(bool follow) { _followPositionY = follow; }
    void SetFollowPositionZ(bool follow) { _followPositionZ = follow; }

    bool IsFollowPositionX() const { return _followPositionX; }
    bool IsFollowPositionY() const { return _followPositionY; }
    bool IsFollowPositionZ() const { return _followPositionZ; }

    void SetFollowRotationX(bool follow) { _followRotationX = follow; }
    void SetFollowRotationY(bool follow) { _followRotationY = follow; }
    void SetFollowRotationZ(bool follow) { _followRotationZ = follow; }

    bool IsFollowRotationX() const { return _followRotationX; }
    bool IsFollowRotationY() const { return _followRotationY; }
    bool IsFollowRotationZ() const { return _followRotationZ; }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
            CEREAL_NVP(_target),
            CEREAL_NVP(_followPositionX),
            CEREAL_NVP(_followPositionY),
            CEREAL_NVP(_followPositionZ),
            CEREAL_NVP(_positionOffset),
            CEREAL_NVP(_positionFollowMode),
            CEREAL_NVP(_positionInterpolationSpeed),
            CEREAL_NVP(_followRotationX),
            CEREAL_NVP(_followRotationY),
            CEREAL_NVP(_followRotationZ)
        );
    }

private:
    TransformRef _target;

    bool _followPositionX = false;
    bool _followPositionY = false;
    bool _followPositionZ = false;
    Vec3 _positionOffset = Vec3::Zero;
    TargetPositionFollowMode _positionFollowMode = TargetPositionFollowMode::Immediate;
    float _positionInterpolationSpeed = 8.f;

    bool _followRotationX = false;
    bool _followRotationY = false;
    bool _followRotationZ = false;
};
