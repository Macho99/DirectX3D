#pragma once
#include "ModelMeshResource.h"
#include "ModelAnimation.h"

class Model : public ResourceBase
	//public enable_shared_from_this<Model>
{
	using Super = ResourceBase;
public:
	Model();
	~Model();

public:
	void ReadMaterial(wstring filename);

	uint32 GetMaterialCount() { return static_cast<uint32>(_materials.size()); }
	vector<MaterialRef>& GetMaterials() { return _materials; }
	MaterialRef GetMaterialByIndex(uint32 index) { return _materials[index]; }
	MaterialRef GetMaterialByName(const wstring& name);

	uint32 GetAnimationCount() { return _animations.size(); }
	vector<ResourceRef<ModelAnimation>>& GetAnimations() { return _animations; }
	ResourceRef<ModelAnimation> GetAnimationByIndex(UINT index) { return (index < 0 || index >= _animations.size()) ? ResourceRef<ModelAnimation>() : _animations[index]; }
	ResourceRef<ModelAnimation> GetAnimationByName(wstring name);

private:
	vector<MaterialRef> _materials;
    ResourceRef<ModelMeshResource> _mesh;
	vector<ResourceRef<ModelAnimation>> _animations;
};