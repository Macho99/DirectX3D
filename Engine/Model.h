#pragma once
#include "ModelMeshResource.h"
#include "ModelAnimation.h"

struct ModelSocket
{
	string name = "ModelSocket";
	int32 boneIndex = -1;
	Matrix localMatrix = Matrix::Identity;

	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(
			CEREAL_NVP(name),
			CEREAL_NVP(boneIndex),
			CEREAL_NVP(localMatrix)
			);
	}
};

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
	virtual int GetVersion() const override;

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
    int32 GetAnimationIndexByName(wstring name) const;
	void AddAnimation(ResourceRef<ModelAnimation> animation) { _animations.push_back(animation); }

	vector<ModelSocket>& GetModelSockets() { return _modelSockets; }
	const vector<ModelSocket>& GetModelSockets() const { return _modelSockets; }
	ModelSocket* GetModelSocketByName(const string& name);
	const ModelSocket* GetModelSocketByName(const string& name) const;

    ModelMeshResource* GetMesh() { return _mesh.Resolve(); }

	virtual bool OnGUI(bool isReadOnly) override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_materials));
        ar(CEREAL_NVP(_mesh));
        ar(CEREAL_NVP(_animations));
		if (_version >= 2)
		{
			ar(CEREAL_NVP(_modelSockets));
		}

		if (Archive::is_loading::value)
			BindCache();
    }

private:
	vector<ResourceRef<Material>> _materials;
    ResourceRef<ModelMeshResource> _mesh;
	vector<ResourceRef<ModelAnimation>> _animations;
	vector<ModelSocket> _modelSockets;
};
