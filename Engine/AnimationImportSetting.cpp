#include "pch.h"
#include "AnimationImportSetting.h"
#include "OnGUIUtils.h"

bool AnimationClipImportSetting::OnGUI()
{
    bool changed = false;
    changed |= OnGUIUtils::DrawBool("Extract Root Motion", &extractRootMotion);
    changed |= OnGUIUtils::DrawBool("Apply Root Position XZ", &applyRootPositionXZ);
    changed |= OnGUIUtils::DrawBool("Apply Root Position Y", &applyRootPositionY);
    changed |= OnGUIUtils::DrawBool("Apply Root Rotation", &applyRootRotation);

    return changed;
}
