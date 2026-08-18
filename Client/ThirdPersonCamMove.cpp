#include "pch.h"
#include "ThirdPersonCamMove.h"
#include "Transform.h"
#include "OnGUIUtils.h"
#include "ModelAnimator.h"

namespace
{
    Vec3 MoveTowards(const Vec3& current, const Vec3& target, float maxDelta)
    {
        Vec3 delta = target - current;
        float distance = delta.Length();
        if (distance <= maxDelta || distance <= 0.0001f)
            return target;

        return current + delta * (maxDelta / distance);
    }

    float MoveTowardsAngle(float current, float target, float maxDelta)
    {
        float delta = std::remainder(target - current, 360.f);
        if (fabsf(delta) <= maxDelta)
            return target;

        return current + std::copysign(maxDelta, delta);
    }
}

void ThirdPersonCamMove::Start()
{
    Transform* pivot = GetRotationPivot();
    if (pivot == nullptr)
        return;

    Vec3 rotation = pivot->GetLocalRotation();
    _pitch = rotation.x;
    _yaw = rotation.y;
    _rotationInitialized = true;
}

void ThirdPersonCamMove::Update()
{
    RotateAroundParent();
    //MoveTarget(TIME->GetDeltaTime());
    SendInputToServer();
}

bool ThirdPersonCamMove::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= OnGUIUtils::DrawComponentRef("Target", _target);
    changed |= OnGUIUtils::DrawFloat("Move Speed", &_moveSpeed, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Sprint Speed", &_sprintSpeed, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Acceleration", &_acceleration, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Deceleration", &_deceleration, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Turn Speed", &_turnSpeed, 1.f);
    changed |= OnGUIUtils::DrawFloat("Mouse Speed", &_mouseSpeed, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Min Pitch", &_minPitch, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Max Pitch", &_maxPitch, 0.1f);

    OnGUIUtils::DrawVec3("Velocity", &_velocity, 0.1f, true);

    _moveSpeed = std::max(0.f, _moveSpeed);
    _sprintSpeed = std::max(_moveSpeed, _sprintSpeed);
    _acceleration = std::max(0.f, _acceleration);
    _deceleration = std::max(0.f, _deceleration);
    _turnSpeed = std::max(0.f, _turnSpeed);
    _mouseSpeed = std::max(0.f, _mouseSpeed);
    if (_minPitch > _maxPitch)
        std::swap(_minPitch, _maxPitch);

    return changed;
}

void ThirdPersonCamMove::RotateAroundParent()
{
    Transform* pivot = GetRotationPivot();
    if (pivot == nullptr)
        return;

    if (!_rotationInitialized)
    {
        Vec3 rotation = pivot->GetLocalRotation();
        _pitch = rotation.x;
        _yaw = rotation.y;
        _rotationInitialized = true;
    }

    if (INPUT->IsMouseCaptured() == false)
        return;

    POINT mouseDelta = INPUT->GetMouseDelta();

    _pitch = std::clamp(_pitch + mouseDelta.y * _mouseSpeed, _minPitch, _maxPitch);
    _yaw = fmodf(_yaw + mouseDelta.x * _mouseSpeed, 360.f);

    Vec3 rotation = pivot->GetLocalRotation();
    rotation.x = _pitch;
    rotation.y = _yaw;
    pivot->SetLocalRotation(rotation);
}

void ThirdPersonCamMove::MoveTarget(float dt)
{
    Transform* target = _target.Resolve();
    Transform* pivot = GetRotationPivot();
    if (target == nullptr || pivot == nullptr || dt <= 0.f)
    {
        _velocity = Vec3::Zero;
        return;
    }

    Vec3 forward = pivot->GetLook();
    forward.y = 0.f;
    if (forward.LengthSquared() > 0.0001f)
        forward.Normalize();

    Vec3 right = pivot->GetRight();
    right.y = 0.f;
    if (right.LengthSquared() > 0.0001f)
        right.Normalize();

    Vec3 moveDirection = Vec3::Zero;
    const bool isMouseCaptured = INPUT->IsMouseCaptured();
    if (isMouseCaptured)
    {
        if (INPUT->GetButton(KEY_TYPE::W)) moveDirection += forward;
        if (INPUT->GetButton(KEY_TYPE::S)) moveDirection -= forward;
        if (INPUT->GetButton(KEY_TYPE::D)) moveDirection += right;
        if (INPUT->GetButton(KEY_TYPE::A)) moveDirection -= right;
    }

    if (moveDirection.LengthSquared() > 1.f)
        moveDirection.Normalize();

    if (moveDirection.LengthSquared() > 0.f)
    {
        Vec3 targetRotation = target->GetRotation();
        float pivotYaw = pivot->GetRotation().y + 180.f;
        targetRotation.y = MoveTowardsAngle(targetRotation.y, pivotYaw, _turnSpeed * dt);
        target->SetRotation(targetRotation);
    }

    float maxSpeed = INPUT->GetButton(KEY_TYPE::LSHIFT) ? _sprintSpeed : _moveSpeed;
    Vec3 desiredVelocity = moveDirection * maxSpeed;
    float rate = moveDirection.LengthSquared() > 0.f ? _acceleration : _deceleration;
    _velocity = MoveTowards(_velocity, desiredVelocity, rate * dt);

    ModelAnimator* animator = target->GetGameObject()->GetModelAnimator();
    if (animator != nullptr)
    {
        Matrix worldToTarget = target->GetWorldMatrix().Invert();
        Vec3 localVelocity = Vec3::TransformNormal(_velocity, worldToTarget);
        localVelocity *= target->GetLocalScale().x;
        Vec2 blendInput = Vec2(localVelocity.x, localVelocity.z);
        blendInput /= _moveSpeed;
        blendInput = -blendInput; // Invert Y for forward/backward
        animator->SetBlendSpaceInput(blendInput);

        if (isMouseCaptured)
        {
            if (INPUT->GetButtonDown(KEY_TYPE::SPACE))
            {
                animator->PlayAnimation("sword and shield jump");
            }
            if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON))
            {
                animator->PlayAnimation("sword and shield slash (2)");
            }
        }
    }

    target->SetPosition(target->GetPosition() + _velocity * dt);
}

void ThirdPersonCamMove::SendInputToServer()
{
    static const vector<KEY_TYPE> keysToCheck = { KEY_TYPE::W, KEY_TYPE::A, KEY_TYPE::S, KEY_TYPE::D, KEY_TYPE::LSHIFT };

    Protocol::C_PLAYER_INPUT inputPacket;
    for (const KEY_TYPE& key : keysToCheck)
    {
        if (INPUT->GetButtonDown(key))
        {
            Protocol::Input* input = inputPacket.add_inputs();
            input->set_down(true);
            input->set_keytype(static_cast<uint32>(key));
        }
        else if (INPUT->GetButtonUp(key))
        {
            Protocol::Input* input = inputPacket.add_inputs();
            input->set_down(false);
            input->set_keytype(static_cast<uint32>(key));
        }
    }

    if (inputPacket.inputs_size() > 0)
    {
        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(inputPacket);
        SERVER_CONNECT->SendPacket(sendBuffer);
    }
}

Transform* ThirdPersonCamMove::GetRotationPivot()
{
    Transform* transform = GetTransform();
    if (transform == nullptr)
        return nullptr;

    Transform* parent = transform->GetParent();
    return parent != nullptr ? parent : transform;
}
