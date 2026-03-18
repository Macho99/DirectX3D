#pragma once
#include "ModelMeshResource.h"
#include "ModelAnimation.h"

class Model : public ResourceBase
	//public enable_shared_from_this<Model>
{
	using Super = ResourceBase;
public:
    static constexpr ResourceType StaticType = ResourceType::Model;
    static string GetExtension() { return ".model"; }
	Model();
    Model(vector<ResourceRef<Material>> materials, ResourceRef<ModelMeshResource> mesh, vector<ResourceRef<ModelAnimation>> animations);
	~Model();

	void BindCache();

public:
	uint32 GetMaterialCount() { return static_cast<uint32>(_materials.size()); }
	vector<ResourceRef<Material>>& GetMaterials() { return _materials; }
	ResourceRef<Material> GetMaterialByIndex(uint32 index) { return _materials[index]; }
	ResourceRef<Material> GetMaterialByName(const wstring& name);

	uint32 GetAnimationCount() { return _animations.size(); }
	vector<ResourceRef<ModelAnimation>>& GetAnimations() { return _animations; }
	ModelAnimation* GetAnimationByIndex(UINT index);
	ModelAnimation* GetAnimationByName(wstring name);
	void AddAnimation(ResourceRef<ModelAnimation> animation) { _animations.push_back(animation); }

    ModelMeshResource* GetMesh() { return _mesh.Resolve(); }

	virtual bool OnGUI(bool isReadOnly) override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(_materials));
        ar(CEREAL_NVP(_mesh));
        ar(CEREAL_NVP(_animations));

		if (Archive::is_loading::value)
			BindCache();
    }

private:
	vector<ResourceRef<Material>> _materials;
    ResourceRef<ModelMeshResource> _mesh;
	vector<ResourceRef<ModelAnimation>> _animations;
};