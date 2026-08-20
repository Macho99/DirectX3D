#include "pch.h"
#include "TargetFollower.h"
#include "OnGUIUtils.h"

namespace
{
    const char* const FollowModeNames[] =
    {
        "Immediate",
        "Interpolated"
    };

    float InterpolateAngle(float current, float target, float ratio)
    {
        return current + std::remainder(target - current, 360.f) * ratio;
    }
}

TargetFollower::TargetFollower() : Super(StaticType)
{
}

TargetFollower::TargetFollower(ComponentType type) : Super(type)
{
}

TargetFollower::~TargetFollower()
{
}

void TargetFollower::Update()
{
    Transform* follower = GetTransform();
    Vec3 targetPosition;
    Vec3 targetRotation;
    if (follower == nullptr || !TryGetTargetPose(OUT targetPosition, OUT targetRotation))
        return;

    if (_followPositionX || _followPositionY || _followPositionZ)
    {
        targetPosition += _positionOffset;
        Vec3 position = follower->GetPosition();

        float followRatio = 1.f;
        if (_positionFollowMode == TargetFollowMode::Interpolated)
            followRatio = 1.f - std::exp(-_positionInterpolationSpeed * DT);

        if (_followPositionX) position.x += (targetPosition.x - position.x) * followRatio;
        if (_followPositionY) position.y += (targetPosition.y - position.y) * followRatio;
        if (_followPositionZ) position.z += (targetPosition.z - position.z) * followRatio;

        follower->SetPosition(position);
    }

    if (_followRotationX || _followRotationY || _followRotationZ)
    {
        Vec3 rotation = follower->GetRotation();

        float followRatio = 1.f;
        if (_rotationFollowMode == TargetFollowMode::Interpolated)
            followRatio = 1.f - std::exp(-_rotationInterpolationSpeed * DT);

        if (_followRotationX) rotation.x = InterpolateAngle(rotation.x, targetRotation.x, followRatio);
        if (_followRotationY) rotation.y = InterpolateAngle(rotation.y, targetRotation.y, followRatio);
        if (_followRotationZ) rotation.z = InterpolateAngle(rotation.z, targetRotation.z, followRatio);

        follower->SetRotation(rotation);
    }
}

bool TargetFollower::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= DrawTargetGUI();
    changed |= OnGUIUtils::DrawBool("Follow Position X", &_followPositionX);
    changed |= OnGUIUtils::DrawBool("Follow Position Y", &_followPositionY);
    changed |= OnGUIUtils::DrawBool("Follow Position Z", &_followPositionZ);
    changed |= OnGUIUtils::DrawVec3("Position Offset", &_positionOffset);
    changed |= OnGUIUtils::DrawEnumCombo("Position Follow Mode", _positionFollowMode,
        FollowModeNames, (int)TargetFollowMode::Max);
    if (_positionFollowMode == TargetFollowMode::Interpolated)
    {
        changed |= OnGUIUtils::DrawFloat("Position Interpolation Speed", &_positionInterpolationSpeed);
        _positionInterpolationSpeed = std::max(0.f, _positionInterpolationSpeed);
    }
    changed |= OnGUIUtils::DrawBool("Follow Rotation X", &_followRotationX);
    changed |= OnGUIUtils::DrawBool("Follow Rotation Y", &_followRotationY);
    changed |= OnGUIUtils::DrawBool("Follow Rotation Z", &_followRotationZ);
    changed |= OnGUIUtils::DrawEnumCombo("Rotation Follow Mode", _rotationFollowMode,
        FollowModeNames, (int)TargetFollowMode::Max);
    if (_rotationFollowMode == TargetFollowMode::Interpolated)
    {
        changed |= OnGUIUtils::DrawFloat("Rotation Interpolation Speed", &_rotationInterpolationSpeed);
        _rotationInterpolationSpeed = std::max(0.f, _rotationInterpolationSpeed);
    }
    return changed;
}

float TargetFollower::GetTargetDistance()
{
    Vec3 targetPosition;
    Vec3 targetRotation;
    if (!TryGetTargetPose(OUT targetPosition, OUT targetRotation))
        return -1;

    return Vec3::Distance(GetTransform()->GetPosition(), targetPosition);
}

bool TargetFollower::TryGetTargetPose(OUT Vec3& position, OUT Vec3& rotation)
{
    Transform* target = _target.Resolve();
    if (target == nullptr || target == GetTransform())
        return false;

    position = target->GetPosition();
    rotation = target->GetRotation();
    return true;
}

bool TargetFollower::DrawTargetGUI()
{
    return OnGUIUtils::DrawComponentRef("Target", _target);
}

void TargetFollower::UpdateImmediateFollow()
{
    TargetFollowMode prevPositionMode = _positionFollowMode;
    TargetFollowMode prevRotationMode = _rotationFollowMode;
    _positionFollowMode = TargetFollowMode::Immediate;
    _rotationFollowMode = TargetFollowMode::Immediate;
    Update();
    _positionFollowMode = prevPositionMode;
    _rotationFollowMode = prevRotationMode;
}
