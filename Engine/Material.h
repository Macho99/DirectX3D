#pragma once
#include "ResourceBase.h"
#include "BindShaderDesc.h"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#define RENDER_QUEUE_LIST \
    X(Opaque) \
    X(Cutout) \
    X(Transparent) \

enum class RenderQueue
{
#define X(name) name,
	RENDER_QUEUE_LIST
#undef X
	Max
};

static const char* RenderQueueNames[] =
{
#define X(name) #name,
	RENDER_QUEUE_LIST
#undef X
};

class Material : public ResourceBase
{
	using Super = ResourceBase;
public:
	static constexpr ResourceType StaticType = ResourceType::Material;
	Material();
	~Material();

	virtual int GetVersion() const override { return 5; }

	Shader* GetShader() { return _shader.Resolve(); }
    ResourceRef<Shader> GetShaderRef() { return _shader; }

	MaterialDesc& GetMaterialDesc() { return _desc; }
	ResourceRef<Texture> GetDiffuseMap() { return _diffuseMap; }
	ResourceRef<Texture> GetNormalMap() { return _normalMap; }
	ResourceRef<Texture> GetSpecularMap() { return _specularMap; }
	//ResourceRef<Texture> GetRandomTex() { return _randomTex; }
    ResourceRef<Texture> GetCubeMap() { return _cubeMap; }

	void SetShader(ResourceRef<Shader> shader);
	void SetDiffuseMap(ResourceRef<Texture> diffuseMap);
	void SetNormalMap(ResourceRef<Texture> normalMap);
	void SetSpecularMap(ResourceRef<Texture> specularMap);
	//void SetRandomTex(ResourceRef<Texture> randomTex);
	void SetCubeMap(ResourceRef<Texture> cubeMap);
    void SetLayerMapArraySRV(ComPtr<ID3D11ShaderResourceView> srv) { _layerMapArraySRV = srv; }
    void SetLayerNormalMapArraySRV(ComPtr<ID3D11ShaderResourceView> srv) { _layerNormalMapArraySRV = srv; }
	void SetRandomTex(bool useRandomTexture) { _useRandomTexture = useRandomTexture; }

	//void SetRenderQueue(RenderQueue renderQueue) { _renderQueue = renderQueue; }
	RenderQueue GetRenderQueue() const { return _renderQueue; }

	void SetCastShadow(bool castShadow) { _castShadow = castShadow; }
	bool GetCastShadow() const { return _castShadow; }
	void SetDrawNormalDepth(bool drawNormalDepth) { _drawNormalDepth = drawNormalDepth; }
	bool GetDrawNormalDepth() const { return _drawNormalDepth; }

    bool IsIncludeInNavMesh() const { return _includeInNavMesh; }

	void Update();
    virtual bool OnGUI(bool isReadOnly) override;
	void Clone(Material* other);
		
    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

		ar(CEREAL_NVP(_desc));
        ar(CEREAL_NVP(_renderQueue));
        ar(CEREAL_NVP(_castShadow));

		if (Archive::is_saving::value || _version >= 5)
		{
			ar(CEREAL_NVP(_drawNormalDepth));
		}

		if (Archive::is_saving::value || _version >= 2)
		{
			ar(CEREAL_NVP(_includeInNavMesh));
		}

		if (Archive::is_saving::value || _version >= 4)
		{
			ar(CEREAL_NVP(_shaderProperties));
		}

        ar(CEREAL_NVP(_shader));
        ar(CEREAL_NVP(_diffuseMap));
        ar(CEREAL_NVP(_normalMap));
        ar(CEREAL_NVP(_specularMap));
        ar(CEREAL_NVP(_cubeMap));
        ar(CEREAL_NVP(_useRandomTexture));
    }
private:
    void InitializeEffectBuffers();
	void SyncShaderProperties();

private:
	friend class MeshRenderer;

	MaterialDesc _desc;
	RenderQueue _renderQueue = RenderQueue::Opaque;
	bool _castShadow = true;
	bool _drawNormalDepth = true;
	bool _includeInNavMesh = true;

	ResourceRef<Shader> _shader;
	ResourceRef<Texture> _diffuseMap;
	ResourceRef<Texture> _normalMap;
	ResourceRef<Texture> _specularMap;
	ResourceRef<Texture> _cubeMap;
    ComPtr<ID3D11ShaderResourceView> _layerMapArraySRV;
    ComPtr<ID3D11ShaderResourceView> _layerNormalMapArraySRV;

	bool _useRandomTexture = false;
	vector<MaterialPropertyValue> _shaderProperties;

	// Cache
    bool _initializedEffectBuffers = false;
	ID3DX11EffectShaderResourceVariable* _diffuseEffectBuffer = nullptr;
	ID3DX11EffectShaderResourceVariable* _normalEffectBuffer = nullptr;
	ID3DX11EffectShaderResourceVariable* _specularEffectBuffer = nullptr;
	ID3DX11EffectShaderResourceVariable* _randomEffectBuffer = nullptr;
	ID3DX11EffectShaderResourceVariable* _cubeMapEffectBuffer = nullptr;
	ID3DX11EffectShaderResourceVariable* _shadowMapEffectBuffer = nullptr;
	ID3DX11EffectShaderResourceVariable* _ssaoMapEffectBuffer = nullptr;
    ID3DX11EffectShaderResourceVariable* _layerMapArrayEffectBuffer = nullptr;
    ID3DX11EffectShaderResourceVariable* _layerNormalMapArrayEffectBuffer = nullptr;
};

using MaterialRef = ResourceRef<Material>;
