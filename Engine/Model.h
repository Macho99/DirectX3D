#pragma once
#include "ModelMeshResource.h"
#include "ModelAnimation.h"
#include "BindShaderDesc.h"

struct AnimTransform
{
	using TransformArrayType = array<Matrix, MAX_MODEL_TRANSFORMS>;
	array<TransformArrayType, MAX_MODEL_KEYFRAMES> transforms;
	array<Matrix, MAX_MODEL_KEYFRAMES> rootTransforms;
};

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
	bool HasValidRenderResources() const;
	bool EnsureAnimationTexture();
	void InvalidateAnimationTexture();
	void ExtractAnimationRootPositionsAndEvents(const fs::path path);
	void LoadAnimationRootPositionsAndEvents(fs::path path);
	ID3D11ShaderResourceView* GetAnimationTransformSRV() const { return _animationTransformSRV.Get(); }
	const vector<AnimTransform>& GetAnimationTransforms() const { return _animTransforms; }

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
	void AddAnimation(ResourceRef<ModelAnimation> animation)
	{
		_animations.push_back(animation);
		InvalidateAnimationTexture();
	}

	vector<ModelSocket>& GetModelSockets() { return _modelSockets; }
	const vector<ModelSocket>& GetModelSockets() const { return _modelSockets; }
	ModelSocket* GetModelSocketByName(const string& name);
	const ModelSocket* GetModelSocketByName(const string& name) const;

    ModelMeshResource* GetMesh() { return _mesh.Resolve(); }

	virtual bool OnGUI(bool isReadOnly) override;
    virtual void OnMenu(bool isReadOnly) override;

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
	void CreateAnimationTransform(uint32 index);

	// Runtime-only animation cache. Intentionally excluded from serialize().
	vector<AnimTransform> _animTransforms;
	vector<AssetId> _animationCacheAssetIds;
	ComPtr<ID3D11Texture2D> _animationTransformTexture;
	ComPtr<ID3D11ShaderResourceView> _animationTransformSRV;

	vector<ResourceRef<Material>> _materials;
    ResourceRef<ModelMeshResource> _mesh;
	vector<ResourceRef<ModelAnimation>> _animations;
	vector<ModelSocket> _modelSockets;
};
