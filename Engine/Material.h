#pragma once
#include "ResourceBase.h"

enum class RenderQueue
{
	Opaque,
	Cutout,
	Transparent,
	Max
};

class Material : public ResourceBase
{
	using Super = ResourceBase;
public:
	Material();
	virtual ~Material();

	Shader* GetShader() { return _shader.get(); }

	MaterialDesc& GetMaterialDesc() { return _desc; }
	shared_ptr<Texture> GetDiffuseMap() { return _diffuseMap; }
	shared_ptr<Texture> GetNormalMap() { return _normalMap; }
	shared_ptr<Texture> GetSpecularMap() { return _specularMap; }
	shared_ptr<Texture> GetRandomTex() { return _randomTex; }

	void SetShader(shared_ptr<Shader> shader);
	void SetDiffuseMap(shared_ptr<Texture> diffuseMap) { _diffuseMap = diffuseMap; }
	void SetNormalMap(shared_ptr<Texture> normalMap) { _normalMap = normalMap; }
	void SetSpecularMap(shared_ptr<Texture> specularMap) { _specularMap = specularMap; }
	void SetRandomTex(shared_ptr<Texture> randomTex) { _randomTex = randomTex; }
	void SetCubeMap(shared_ptr<Texture> cubeMap) { _cubeMap = cubeMap; }
    void SetLayerMapArraySRV(shared_ptr<ID3D11ShaderResourceView> srv) { _layerMapArraySRV = srv; }

	void SetRenderQueue(RenderQueue renderQueue) { _renderQueue = renderQueue; }
	RenderQueue GetRenderQueue() { return _renderQueue; }

	void SetCastShadow(bool castShadow) { _castShadow = castShadow; }
	bool GetCastShadow() { return _castShadow; }

	void Update();

	shared_ptr<Material> Clone();

private:
	friend class MeshRenderer;

	MaterialDesc _desc;
	RenderQueue _renderQueue = RenderQueue::Opaque;
	bool _castShadow = true;

	shared_ptr<Shader> _shader;
	shared_ptr<Texture> _diffuseMap;
	shared_ptr<Texture> _normalMap;
	shared_ptr<Texture> _specularMap;
	shared_ptr<Texture> _randomTex;
	shared_ptr<Texture> _cubeMap;
    shared_ptr<ID3D11ShaderResourceView> _layerMapArraySRV;

	// Cache
	ComPtr<ID3DX11EffectShaderResourceVariable> _diffuseEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _normalEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _specularEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _randomEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _cubeMapEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _shadowMapEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _ssaoMapEffectBuffer;
    ComPtr<ID3DX11EffectShaderResourceVariable> _layerMapArrayEffectBuffer;
};

