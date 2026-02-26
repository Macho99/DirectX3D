#pragma once
#include "ResourceBase.h"

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

class ModelAnimation : public ResourceBase
{
    using Super = ResourceBase;
public:
    ModelAnimation();
    ~ModelAnimation();

	void ReadAnimation(wstring filename);

	shared_ptr<ModelKeyframe> GetKeyframe(const wstring& name);
	wstring GetName() { return name; }

private:
	wstring name = L"empty";
	float duration = 0.f;
	float frameRate = 0.f;
	uint32 frameCount = 0;
	unordered_map<wstring, shared_ptr<ModelKeyframe>> keyframes;
};

