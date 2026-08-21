#include "pch.h"
#include "Character.h"
#include "ModelAnimator.h"
#include "OnGUIUtils.h"
#include "MathLibrary/MathUtils.h"

void Character::Start()
{
    EnsureAnimator();
}

void Character::Update()
{
    Transform* transform = GetTransform();
    const float interpolationValue = _interpolationSpeed * TIME->GetDeltaTime();
    
    if (_debugIgnorePositionUpdate == false)
    {
        const float elapsedSinceServerUpdate = TIME->GetGameTime() - _lastServerTime;
        const Vec3 estimatedPosition = _serverPosition
            + Vec3(_serverVelocity.x, 0.f, _serverVelocity.y) * elapsedSinceServerUpdate;
        Vec3 interpolatedPosition = Vec3::Lerp(transform->GetPosition(), estimatedPosition, interpolationValue);
        transform->SetPosition(interpolatedPosition);
    }

    float interpolatedYaw = MathUtils::MoveTowardsAngle(transform->GetRotation().y, _serverYaw, _yawRotationSpeed * DT);
    transform->SetRotation(Vec3(0, interpolatedYaw, 0));
    
    ModelAnimator* animator = _animator.Resolve();
    if (animator)
    {
        Vec2 interpolatedBlendInput = Vec2::Lerp(_animator.Resolve()->GetBlendSpaceInput(), _serverBlendInput, interpolationValue);
        //const Vec2 localVelocity = MathUtils::InverseRotateByYaw(_serverVelocity, _serverYaw + 180.f);
        //const Vec2 blendInput = Vec2(localVelocity.x, localVelocity.y) / 2.5f;
        //const Vec2 interpolatedBlendInput = MathUtils::MoveTowards(animator->GetBlendSpaceInput(), blendInput, interpolationValue);
        animator->SetBlendSpaceInput(interpolatedBlendInput);
    }
}

bool Character::OnGUI()
{
    bool changed = false;

    changed |= OnGUIUtils::DrawVec3("Server Position", &_serverPosition);
    changed |= OnGUIUtils::DrawFloat("Server Yaw", &_serverYaw);
    changed |= OnGUIUtils::DrawVec2("Server Blend Input", &_serverBlendInput);
    changed |= OnGUIUtils::DrawFloat("Interpolation Speed", &_interpolationSpeed, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Yaw Rotation Speed", &_yawRotationSpeed, 0.01f);
    changed |= OnGUIUtils::DrawBool("Debug Ignore Server Update", &_debugIgnorePositionUpdate);

    return changed;
}

void Character::EnsureAnimator()
{
    if (_animator.Resolve() == nullptr)
    {
        _animator = GetGameObject()->GetComponent<ModelAnimator>();
    }
}

void Character::UpdateServerTransform(Vec3 position, float yaw, Vec2 velocity, Vec2 blendInput, bool updateImmediate)
{
    _serverPosition = position;
    _serverYaw = yaw;
    _serverVelocity = velocity;
    _serverBlendInput = blendInput;
    _lastServerTime = TIME->GetGameTime();

    if (updateImmediate)
    {
        Transform* transform = GetTransform();
        transform->SetPosition(_serverPosition);
        transform->SetRotation(Vec3(0, _serverYaw, 0));
        ModelAnimator* animator = _animator.Resolve();
        if (animator)
        {
            animator->SetBlendSpaceInput(blendInput);
        }
    }
}

void Character::PlayAnimation(int animationIndex)
{
    ModelAnimator* animator = _animator.Resolve();
    if (animator)
    {
        animator->PlayAnimation(animationIndex);
    }
}

void Character::SetHp(int hp)
{
    _curHp = hp;
}
