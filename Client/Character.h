#pragma once
#include <MonoBehaviour.h>

class Character : public MonoBehaviour
{
public:
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    void EnsureAnimator();
    void UpdateServerTransform(Vec3 position, float yaw, Vec2 velocity, Vec2 blendInput, bool updateImmediate);
    void PlayAnimation(int animationIndex);

private:
    ComponentRef<ModelAnimator> _animator;

    Vec3 _serverPosition;
    float _serverYaw;
    Vec2 _serverVelocity;
    Vec2 _serverBlendInput;

    float _interpolationSpeed = 10.f;
    float _yawRotationSpeed = 360.f;
    float _lastServerTime = 0.f;

    bool _debugIgnorePositionUpdate = false;
};
