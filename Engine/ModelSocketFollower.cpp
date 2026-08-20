#include "pch.h"
#include "ModelSocketFollower.h"
#include "ModelAnimator.h"
#include "OnGUIUtils.h"
#include "Transform.h"
#include "MathLibrary/MathUtils.h"

ModelSocketFollower::ModelSocketFollower() : Super(StaticType)
{
}

ModelSocketFollower::~ModelSocketFollower()
{
}

void ModelSocketFollower::SetModelAnimator(ModelAnimator* animator)
{
    _modelAnimator = ComponentRef<ModelAnimator>(animator);
}

ModelAnimator* ModelSocketFollower::GetModelAnimator() const
{
    return _modelAnimator.Resolve();
}

bool ModelSocketFollower::TryGetTargetPose(OUT Vec3& position, OUT Vec3& rotation)
{
    ModelAnimator* animator = _modelAnimator.Resolve();
    if (animator == nullptr || animator->GetTransform() == GetTransform() || _socketName.empty())
        return false;

    Matrix socketWorldMatrix;
    if (!animator->TryGetModelSocketWorldMatrix(_socketName, OUT socketWorldMatrix))
        return false;

    Vec3 scale;
    Quaternion quaternion;
    if (!socketWorldMatrix.Decompose(OUT scale, OUT quaternion, OUT position))
        return false;

    rotation = MathUtils::RadToDeg(Transform::ToEulerAngles(quaternion));
    return true;
}

bool ModelSocketFollower::DrawTargetGUI()
{
    bool changed = OnGUIUtils::DrawComponentRef("Model Animator", _modelAnimator);
    changed |= OnGUIUtils::DrawString("Socket Name", &_socketName);
    return changed;
}
