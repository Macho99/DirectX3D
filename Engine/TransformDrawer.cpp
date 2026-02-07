#include "pch.h"
#include "TransformDrawer.h"
#include "MathUtils.h"

bool TransformDrawer::DrawImpl(Transform& component)
{
    Vec3 position = component.GetLocalPosition();
    if (ImGui::DragFloat3("Position", &position.x, _dragSpeed))
    {
        component.SetLocalPosition(position);
    }
    Vec3 radRotation = component.GetLocalRotation();
    Vec3 degRotation = MathUtils::RadToDeg(radRotation);
    if (ImGui::DragFloat3("Rotation", &degRotation.x, _dragSpeed))
    {
        component.SetLocalRotation(MathUtils::DegToRad(degRotation));
    }
    Vec3 scale = component.GetLocalScale();
    if (ImGui::DragFloat3("Scale", &scale.x, _dragSpeed))
    {
        component.SetLocalScale(scale);
    }

    return false;
}
