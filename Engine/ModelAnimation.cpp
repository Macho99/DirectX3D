#include "pch.h"
#include "ModelAnimation.h"
#include "FileUtils.h"

ModelAnimation::ModelAnimation()
    : Super(ResourceType::Animation)
{
}

ModelAnimation::~ModelAnimation()
{
}

void ModelAnimation::ReadAnimation(wstring fullPath)
{
	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(fullPath, FileMode::Read);

	name = Utils::ToWString(file->Read<string>());
	duration = file->Read<float>();
	frameRate = file->Read<float>();
	frameCount = file->Read<uint32>();

	uint32 keyframesCount = file->Read<uint32>();

	for (uint32 i = 0; i < keyframesCount; i++)
	{
		shared_ptr<ModelKeyframe> keyframe = make_shared<ModelKeyframe>();
		keyframe->boneName = Utils::ToWString(file->Read<string>());

		uint32 size = file->Read<uint32>();

		if (size > 0)
		{
			keyframe->transforms.resize(size);
			void* ptr = &keyframe->transforms[0];
			file->Read(&ptr, sizeof(ModelKeyframeData) * size);
		}

		keyframes[keyframe->boneName] = keyframe;
	}
}


shared_ptr<ModelKeyframe> ModelAnimation::GetKeyframe(const wstring& name)
{
	auto findIt = keyframes.find(name);

	if (findIt == keyframes.end())
	{
		return nullptr;
	}

	return findIt->second;
}
