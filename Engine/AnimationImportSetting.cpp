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
    changed |= OnGUIUtils::DrawBool("Is Dead Animation", &isDeadAnimation);

    return changed;
}

bool AnimationEvent::OnGUI()
{
    bool changed = false;
    changed |= OnGUIUtils::DrawString("Event Name", &eventName);
    changed |= OnGUIUtils::DrawUInt32("Frame", &frame, 1);
    changed |= OnGUIUtils::DrawFloat("Float Param", &floatParam, 0.1f);
    changed |= OnGUIUtils::DrawInt32("Int Param", &intParam, 1);
    changed |= OnGUIUtils::DrawBool("Bool Param", &boolParam);

    return changed;
}
