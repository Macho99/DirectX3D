#pragma once
#include "ResourceBase.h"
#include "AnimationImportSetting.h"

struct ModelKeyframeData
{
	float time;
	Vec3 scale;
	Quaternion rotation;
	Vec3 translation;
};

struct ModelKeyframe
{
	wstring boneName;
	vector<ModelKeyframeData> transforms;
};

class AnimationOverrideMeta;

class ModelAnimation : public ResourceBase
{
    using Super = ResourceBase;
public:
	static constexpr ResourceType StaticType = ResourceType::Animation;
    ModelAnimation();
    ~ModelAnimation();

	void ReadAnimation(wstring filename);

	shared_ptr<ModelKeyframe> GetKeyframe(const wstring& name);
	shared_ptr<const ModelKeyframe> GetKeyframe(const wstring& name) const;
	const wstring& GetName() const { return name; }
    float GetFrameRate() const { return frameRate; }
    uint32 GetFrameCount() const { return frameCount; }

    void SetAnimationClipImportSetting(const AnimationClipImportSetting& setting) { clipImportSetting = setting; }
    const AnimationClipImportSetting& GetAnimationClipImportSetting() const { return clipImportSetting; }

protected:
    void ResetAnimation(
        const wstring& animationName,
        float animationFrameRate,
        uint32 animationFrameCount);
    void SetKeyframe(const shared_ptr<ModelKeyframe>& keyframe);

private:
	wstring name = L"empty";
	float frameRate = 0.f;
	uint32 frameCount = 0;
	unordered_map<wstring, shared_ptr<ModelKeyframe>> keyframes;

    AnimationClipImportSetting clipImportSetting;

    friend class AnimationOverrideMeta;
};

