#include "pch.h"
#include "AnimationOverrideMeta.h"
#include "Model.h"
#include "ModelAnimation.h"
#include "ModelMesh.h"
#include "ModelMeshResource.h"
#include "OnGUIUtils.h"

namespace
{
    ModelKeyframeData SampleKeyframe(
        const ModelKeyframe& keyframe,
        float normalizedTime,
        float targetTime)
    {
        const vector<ModelKeyframeData>& transforms = keyframe.transforms;
        if (transforms.size() == 1)
        {
            ModelKeyframeData result = transforms.front();
            result.time = targetTime;
            return result;
        }

        const float sourcePosition =
            clamp(normalizedTime, 0.f, 1.f) * static_cast<float>(transforms.size() - 1);
        const size_t firstIndex = static_cast<size_t>(floor(sourcePosition));
        const size_t secondIndex = min(firstIndex + 1, transforms.size() - 1);
        const float ratio = sourcePosition - static_cast<float>(firstIndex);

        const ModelKeyframeData& first = transforms[firstIndex];
        const ModelKeyframeData& second = transforms[secondIndex];

        ModelKeyframeData result;
        result.time = targetTime;
        result.scale = Vec3::Lerp(first.scale, second.scale, ratio);
        result.rotation = Quaternion::Slerp(first.rotation, second.rotation, ratio);
        result.translation = Vec3::Lerp(first.translation, second.translation, ratio);
        return result;
    }

    shared_ptr<ModelKeyframe> ResampleKeyframe(
        const ModelKeyframe& source,
        uint32 targetFrameCount)
    {
        shared_ptr<ModelKeyframe> result = make_shared<ModelKeyframe>();
        result->boneName = source.boneName;
        result->transforms.reserve(targetFrameCount);

        for (uint32 frameIndex = 0; frameIndex < targetFrameCount; ++frameIndex)
        {
            const float normalizedTime = targetFrameCount > 1
                ? static_cast<float>(frameIndex) / static_cast<float>(targetFrameCount - 1)
                : 0.f;
            result->transforms.push_back(
                SampleKeyframe(source, normalizedTime, static_cast<float>(frameIndex)));
        }

        return result;
    }
}

unique_ptr<ResourceBase> AnimationOverrideMeta::LoadResource(AssetId assetId) const
{
    ModelAnimation* baseAnimation = _baseAnimation.Resolve();
    ResourceRef<Model> modelRef = _model;
    Model* resolvedModel = modelRef.Resolve();
    ModelMeshResource* modelMesh = resolvedModel != nullptr ? resolvedModel->GetMesh() : nullptr;

    if(baseAnimation == nullptr || modelMesh == nullptr || _overrideLayers.empty())
        return make_unique<ModelAnimation>();

    unique_ptr<ModelAnimation> result = make_unique<ModelAnimation>();
    wstring error;
    if (!BuildAnimation(*result, *modelMesh, *baseAnimation, OUT error))
    {
        DBG->LogW(L"[AnimationOverrideMeta] " + error);
        return make_unique<ModelAnimation>();
    }

    return result;
}

bool AnimationOverrideMeta::BuildAnimation(
    ModelAnimation& output,
    const ModelMeshResource& modelMesh,
    const ModelAnimation& baseAnimation,
    wstring& error) const
{
    const uint32 targetFrameCount = baseAnimation.GetFrameCount();
    if (targetFrameCount == 0 || baseAnimation.GetFrameRate() <= 0.f)
    {
        error = L"Base animation must have at least one frame and a positive frame rate.";
        return false;
    }

    const vector<shared_ptr<ModelBone>>& bones = modelMesh.GetBones();

    vector<const ModelAnimation*> animationByBone(bones.size(), &baseAnimation);
    for (const AnimationOverrideMetaLayer& layer : _overrideLayers)
    {
        ResourceRef<ModelAnimation> animationRef = layer.animation;
        const ModelAnimation* overrideAnimation = animationRef.Resolve();
        const wstring rootBoneName = Utils::ToWString(layer.rootBoneName);
        if (overrideAnimation == nullptr)
        {
            error = L"Override animation is null for root bone: " + rootBoneName;
            return false;
        }

        size_t rootIndex = bones.size();
        for (size_t boneIndex = 0; boneIndex < bones.size(); ++boneIndex)
        {
            if (bones[boneIndex]->name == rootBoneName)
            {
                rootIndex = boneIndex;
                break;
            }
        }

        if (rootIndex == bones.size())
        {
            error = L"Override root bone was not found: " + rootBoneName;
            return false;
        }

        vector<size_t> pendingBones = { rootIndex };
        vector<bool> visited(bones.size(), false);
        while (!pendingBones.empty())
        {
            const size_t boneIndex = pendingBones.back();
            pendingBones.pop_back();
            if (visited[boneIndex])
                continue;

            visited[boneIndex] = true;
            animationByBone[boneIndex] = overrideAnimation;
            for (int32 childIndex : bones[boneIndex]->childrenIndices)
                pendingBones.push_back(static_cast<size_t>(childIndex));
        }
    }

    output.ResetAnimation(GetAssetPath().stem().wstring(), baseAnimation.GetFrameRate(), targetFrameCount);

    for (size_t boneIndex = 0; boneIndex < bones.size(); ++boneIndex)
    {
        const wstring& boneName = bones[boneIndex]->name;
        shared_ptr<const ModelKeyframe> source =
            animationByBone[boneIndex]->GetKeyframe(boneName);

        // Partial override clips keep the base channel. If neither animation has
        // the channel, ModelAnimator falls back to the bind pose.
        if (source == nullptr || source->transforms.empty())
            source = baseAnimation.GetKeyframe(boneName);
        if (source == nullptr || source->transforms.empty())
            continue;

        output.SetKeyframe(ResampleKeyframe(*source, targetFrameCount));
    }

    output.SetAnimationClipImportSetting(clipImportSetting);
    output.SetAnimationEvents(animationEvents);
    return true;
}

bool AnimationOverrideMeta::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= OnGUIUtils::DrawResourceRef("Model", _model);
    changed |= OnGUIUtils::DrawResourceRef("Base Animation", _baseAnimation);

    uint32 layerCount = static_cast<uint32>(_overrideLayers.size());
    if (OnGUIUtils::DrawUInt32("Override Count", &layerCount, 1.f))
    {
        _overrideLayers.resize(layerCount);
        changed = true;
    }

    Model* resolvedModel = _model.Resolve();
    ModelMeshResource* modelMesh = resolvedModel != nullptr ? resolvedModel->GetMesh() : nullptr;

    for (size_t layerIndex = 0; layerIndex < _overrideLayers.size(); ++layerIndex)
    {
        AnimationOverrideMetaLayer& layer = _overrideLayers[layerIndex];
        ImGui::PushID(static_cast<int>(layerIndex));

        const string header = "Override " + to_string(layerIndex);
        if (ImGui::TreeNode(header.c_str()))
        {
            changed |= OnGUIUtils::DrawResourceRef("Animation", layer.animation);

            const string preview = layer.rootBoneName.empty() ? "None" : layer.rootBoneName;
            if (ImGui::BeginCombo("Root Bone", preview.c_str()))
            {
                if (modelMesh != nullptr)
                {
                    const vector<shared_ptr<ModelBone>>& bones = modelMesh->GetBones();
                    for (const shared_ptr<ModelBone>& bone : bones)
                    {
                        if (bone == nullptr)
                            continue;

                        const string boneName = Utils::ToString(bone->name);
                        const bool selected = layer.rootBoneName == boneName;
                        if (ImGui::Selectable(boneName.c_str(), selected))
                        {
                            layer.rootBoneName = boneName;
                            changed = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    return changed;
}
