#include "pch.h"
#include "AnimationMeta.h"
#include "ModelAnimation.h"

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
    return animation;
}

bool AnimationMeta::OnGUI()
{
    bool changed = Super::OnGUI();

    changed |= clipImportSetting.OnGUI();

    return changed;
}

int AnimationMeta::GetVersion() const
{
    return 2;
}
