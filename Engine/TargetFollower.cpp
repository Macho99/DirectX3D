#include "pch.h"
#include "TargetFollower.h"
#include "OnGUIUtils.h"

namespace
{
    const char* const PositionFollowModeNames[] =
    {
        "Immediate",
        "Interpolated"
    };
}

TargetFollower::TargetFollower() : Super(StaticType)
{
}

TargetFollower::~TargetFollower()
{
}

void TargetFollower::Update()
{
    Transform* target = _target.Resolve();
    Transform* follower = GetTransform();
    if (target == nullptr || target == follower)
        return;

    if (_followPositionX || _followPositionY || _followPositionZ)
    {
        const Vec3 targetPosition = target->GetPosition() + _positionOffset;
        Vec3 position = follower->GetPosition();

        float followRatio = 1.f;
        if (_positionFollowMode == TargetPositionFollowMode::Interpolated)
            followRatio = 1.f - std::exp(-_positionInterpolationSpeed * DT);

        if (_followPositionX) position.x += (targetPosition.x - position.x) * followRatio;
        if (_followPositionY) position.y += (targetPosition.y - position.y) * followRatio;
        if (_followPositionZ) position.z += (targetPosition.z - position.z) * followRatio;

        follower->SetPosition(position);
    }

    if (_followRotationX || _followRotationY || _followRotationZ)
    {
        const Vec3 targetRotation = target->GetRotation();
        Vec3 rotation = follower->GetRotation();

        if (_followRotationX) rotation.x = targetRotation.x;
        if (_followRotationY) rotation.y = targetRotation.y;
        if (_followRotationZ) rotation.z = targetRotation.z;

        follower->SetRotation(rotation);
    }
}

bool TargetFollower::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= OnGUIUtils::DrawComponentRef("Target", _target);
    changed |= OnGUIUtils::DrawBool("Follow Position X", &_followPositionX);
    changed |= OnGUIUtils::DrawBool("Follow Position Y", &_followPositionY);
    changed |= OnGUIUtils::DrawBool("Follow Position Z", &_followPositionZ);
    changed |= OnGUIUtils::DrawVec3("Position Offset", &_positionOffset);
    changed |= OnGUIUtils::DrawEnumCombo("Position Follow Mode", _positionFollowMode,
        PositionFollowModeNames, (int)TargetPositionFollowMode::Max);
    if (_positionFollowMode == TargetPositionFollowMode::Interpolated)
    {
        changed |= OnGUIUtils::DrawFloat("Position Interpolation Speed", &_positionInterpolationSpeed);
        _positionInterpolationSpeed = std::max(0.f, _positionInterpolationSpeed);
    }
    changed |= OnGUIUtils::DrawBool("Follow Rotation X", &_followRotationX);
    changed |= OnGUIUtils::DrawBool("Follow Rotation Y", &_followRotationY);
    changed |= OnGUIUtils::DrawBool("Follow Rotation Z", &_followRotationZ);
    return changed;
}
