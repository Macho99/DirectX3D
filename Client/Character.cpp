#include "pch.h"
#include "Character.h"
#include "ModelAnimator.h"

void Character::Start()
{
    _animator = GetGameObject()->GetComponent<ModelAnimator>();
}

void Character::Update()
{
    Transform* transform = GetTransform();
    transform->SetPosition(_serverPosition);
    transform->SetRotation(Vec3(0, _serverYaw, 0));
    
    ModelAnimator* animator = _animator.Resolve();
    if (animator)
    {
        animator->SetBlendSpaceInput(_serverBlendInput);
    }
}

void Character::UpdateServerTransform(Vec3 position, float yaw, Vec2 blendInput)
{
    _serverPosition = position;
    _serverYaw = yaw;
    _serverBlendInput = blendInput;
}