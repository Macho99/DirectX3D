#pragma once
#include "MonoBehaviour.h"
class EditorCamController : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(EditorCamController)

public:
    virtual void Awake() override;
    virtual void Update() override;

private:
    Vec3 _initLocalPos = Vec3::Zero;
    ComponentRef<class TessTerrain> _terrain = nullptr;
};

