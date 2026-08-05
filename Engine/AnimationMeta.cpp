#include "pch.h"
#include "AnimationMeta.h"
#include "ModelAnimation.h"
#include "OnGUIUtils.h"

unique_ptr<ResourceBase> AnimationMeta::LoadResource(AssetId assetId) const
{
    unique_ptr<ResourceBase> animation = Super::LoadResource(assetId);

    if (animation == nullptr)
    {
        return animation;
    }

    if (animation->GetType() != ResourceType::Animation)
    {
        ASSERT(false, "AnimationMeta::LoadResource: Loaded resource is not of type Animation.");
        return animation;
    }

    ModelAnimation* modelAnimation = static_cast<ModelAnimation*>(animation.get());
    modelAnimation->SetAnimationClipImportSetting(clipImportSetting);
    modelAnimation->SetAnimationEvents(animationEvents);
    return animation;
}

bool AnimationMeta::OnGUI()
{
    bool changed = Super::OnGUI();

    changed |= clipImportSetting.OnGUI();

    uint32 eventCount = static_cast<uint32>(animationEvents.size());
    if (OnGUIUtils::DrawUInt32("Event Count", &eventCount, 1.f))
    {
        animationEvents.resize(eventCount);
        changed = true;
    }

    for (int eventIndex = 0; eventIndex < animationEvents.size(); ++eventIndex)
    {
        AnimationEvent& animationEvent = animationEvents[eventIndex];
        ImGui::PushID(eventIndex);

        string header = to_string(eventIndex);
        if (!animationEvent.eventName.empty())
            header += ": " + animationEvent.eventName;
        header += "###AnimationEvent";

        if (ImGui::TreeNodeEx(header.c_str()))
        {
            changed |= animationEvent.OnGUI();
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    return changed;
}

int AnimationMeta::GetVersion() const
{
    return 2;
}
