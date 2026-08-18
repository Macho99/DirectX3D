#pragma once
#include <MonoBehaviour.h>

class Character : public MonoBehaviour
{
public:
    virtual void Start() override;
    virtual void Update() override;

    void UpdateServerTransform(Vec3 position, float yaw, Vec2 blendInput);

private:
    ComponentRef<ModelAnimator> _animator;

    Vec3 _serverPosition;
    float _serverYaw;
    Vec2 _serverBlendInput;
};