#pragma once
#include "ResourceBase.h"

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
	Material();
	~Material();

	Shader* GetShader() { return _shader.Resolve(); }

	MaterialDesc& GetMaterialDesc() { return _desc; }
	ResourceRef<Texture> GetDiffuseMap() { return _diffuseMap; }
	ResourceRef<Texture> GetNormalMap() { return _normalMap; }
	ResourceRef<Texture> GetSpecularMap() { return _specularMap; }
	//ResourceRef<Texture> GetRandomTex() { return _randomTex; }

	void SetShader(ResourceRef<Shader> shader);
	void SetDiffuseMap(ResourceRef<Texture> diffuseMap);
	void SetNormalMap(ResourceRef<Texture> normalMap);
	void SetSpecularMap(ResourceRef<Texture> specularMap);
	//void SetRandomTex(ResourceRef<Texture> randomTex);
	void SetCubeMap(ResourceRef<Texture> cubeMap);
    void SetLayerMapArraySRV(ComPtr<ID3D11ShaderResourceView> srv) { _layerMapArraySRV = srv; }
	void SetRandomTex(bool useRandomTexture) { _useRandomTexture = useRandomTexture; }

	void SetRenderQueue(RenderQueue renderQueue) { _renderQueue = renderQueue; }
	RenderQueue GetRenderQueue() { return _renderQueue; }

	void SetCastShadow(bool castShadow) { _castShadow = castShadow; }
	bool GetCastShadow() { return _castShadow; }

	void Update();
    virtual bool OnGUI(bool isReadOnly) override;
	//shared_ptr<Material> Clone();
		
    template<class Archive>
    void serialize(Archive& ar)
    {
		ar(CEREAL_NVP(_desc));
        ar(CEREAL_NVP(_renderQueue));
        ar(CEREAL_NVP(_castShadow));
        ar(CEREAL_NVP(_shader));
        ar(CEREAL_NVP(_diffuseMap));
        ar(CEREAL_NVP(_normalMap));
        ar(CEREAL_NVP(_specularMap));
        ar(CEREAL_NVP(_cubeMap));
        ar(CEREAL_NVP(_useRandomTexture));
    }
private:
    void InitializeEffectBuffers();

private:
	friend class MeshRenderer;

	MaterialDesc _desc;
	RenderQueue _renderQueue = RenderQueue::Opaque;
	bool _castShadow = true;

	ResourceRef<Shader> _shader;
	ResourceRef<Texture> _diffuseMap;
	ResourceRef<Texture> _normalMap;
	ResourceRef<Texture> _specularMap;
	ResourceRef<Texture> _cubeMap;
    ComPtr<ID3D11ShaderResourceView> _layerMapArraySRV;

	bool _useRandomTexture = false;

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
};

using MaterialRef = ResourceRef<Material>;