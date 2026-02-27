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

	void BindCache();

public:
	uint32 GetMaterialCount() { return static_cast<uint32>(_materials.size()); }
	vector<ResourceRef<Material>>& GetMaterials() { return _materials; }
	ResourceRef<Material> GetMaterialByIndex(uint32 index) { return _materials[index]; }
	ResourceRef<Material> GetMaterialByName(const wstring& name);

	uint32 GetAnimationCount() { return _animations.size(); }
	vector<ResourceRef<ModelAnimation>>& GetAnimations() { return _animations; }
	ModelAnimation* GetAnimationByIndex(UINT index) { return (index < 0 || index >= _animations.size()) ? nullptr : _animations[index].Resolve(); }
	ModelAnimation* GetAnimationByName(wstring name);

    ModelMeshResource* GetMesh() { return _mesh.Resolve(); }

private:
	vector<ResourceRef<Material>> _materials;
    ResourceRef<ModelMeshResource> _mesh;
	vector<ResourceRef<ModelAnimation>> _animations;
};