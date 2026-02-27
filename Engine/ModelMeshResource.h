#pragma once
#include "ResourceBase.h"
struct ModelBone;
struct ModelMesh;

class ModelMeshResource : public ResourceBase
{
    using Super = ResourceBase;
public:
	ModelMeshResource();
	~ModelMeshResource();

	void ReadModel(wstring filename);

	uint32 GetMeshCount() { return static_cast<uint32>(_meshes.size()); }
	vector<shared_ptr<ModelMesh>>& GetMeshes() { return _meshes; }
	shared_ptr<ModelMesh> GetMeshByIndex(uint32 index) { return _meshes[index]; }
	shared_ptr<ModelMesh> GetMeshByName(const wstring& name);

	uint32 GetBoneCount() { return static_cast<uint32>(_bones.size()); }
	vector<shared_ptr<ModelBone>>& GetBones() { return _bones; }
	shared_ptr<ModelBone> GetBoneByIndex(uint32 index) { return (index < 0 || index >= _bones.size() ? nullptr : _bones[index]); }
	shared_ptr<ModelBone> GetBoneByName(const wstring& name);

	void BindCacheInfo(vector<ResourceRef<Material>> materials);

private:
	shared_ptr<ModelBone> _root;
	vector<shared_ptr<ModelBone>> _bones;
	vector<shared_ptr<ModelMesh>> _meshes;
};