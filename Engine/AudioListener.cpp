#include "pch.h"
#include "AudioListener.h"
#include "SoundManager.h"
#include "Transform.h"

void AudioListener::Start()
{
    OnEnable();
}

void AudioListener::LateUpdate()
{
    ApplyTransform();
}

void AudioListener::OnEnable()
{
    ma_engine* engine = GET_SINGLE(SoundManager)->GetEngine();
    if (engine == nullptr)
        return;

    ma_engine_listener_set_enabled(engine, 0, MA_TRUE);
    _hasPreviousPosition = false;
    ApplyTransform();
}

void AudioListener::OnDisable()
{
    SoundManager* soundManager = GET_SINGLE(SoundManager);
    if (soundManager->IsInitialized())
        ma_engine_listener_set_enabled(soundManager->GetEngine(), 0, MA_FALSE);
}

void AudioListener::OnDestroy()
{
    OnDisable();
}

void AudioListener::ApplyTransform()
{
    ma_engine* engine = GET_SINGLE(SoundManager)->GetEngine();
    Transform* transform = GetTransform();
    if (engine == nullptr || transform == nullptr || !IsActiveAndEnabled())
        return;

    const Vec3 position = transform->GetPosition();
    const Vec3 forward = transform->GetLook();
    const Vec3 up = transform->GetUp();
    ma_engine_listener_set_position(engine, 0, position.x, position.y, -position.z);
    ma_engine_listener_set_direction(engine, 0, forward.x, forward.y, -forward.z);
    ma_engine_listener_set_world_up(engine, 0, up.x, up.y, -up.z);

    Vec3 velocity = Vec3::Zero;
    if (_hasPreviousPosition && DT > 0.f)
        velocity = (position - _previousPosition) / DT;
    ma_engine_listener_set_velocity(engine, 0, velocity.x, velocity.y, -velocity.z);

    _previousPosition = position;
    _hasPreviousPosition = true;
}
