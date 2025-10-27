#include "pch.h"
#include "Material.h"

Material::Material() : Super(ResourceType::Material)
{
}

Material::~Material()
{
}

void Material::SetShader(shared_ptr<Shader> shader)
{
	_shader = shader;

	_diffuseEffectBuffer = _shader->GetSRV("DiffuseMap");
	_normalEffectBuffer = _shader->GetSRV("NormalMap");
	_specularEffectBuffer = _shader->GetSRV("SpecularMap");
	_randomEffectBuffer = _shader->GetSRV("RandomMap");
	_cubeMapEffectBuffer = _shader->GetSRV("CubeMap");
	_shadowMapEffectBuffer = _shader->GetSRV("ShadowMap");
	_ssaoMapEffectBuffer = _shader->GetSRV("SsaoMap");
    _layerMapArrayEffectBuffer = _shader->GetSRV("LayerMapArray");
}

void Material::Update()
{
	if (_shader == nullptr)
		return;

	_shader->PushMaterialData(_desc);

	if (_diffuseMap)
	{
		_diffuseEffectBuffer->SetResource(_diffuseMap->GetComPtr().Get());
	}

	if (_normalMap)
	{
		_normalEffectBuffer->SetResource(_normalMap->GetComPtr().Get());
	}

	if (_specularMap)
	{
		_specularEffectBuffer->SetResource(_specularMap->GetComPtr().Get());
	}

	if (_randomTex)
	{
		_randomEffectBuffer->SetResource(_randomTex->GetComPtr().Get());
	}

	if (_cubeMap)
	{
		_cubeMapEffectBuffer->SetResource(_cubeMap->GetComPtr().Get());
	}

	if (_castShadow)
	{
		_shadowMapEffectBuffer->SetResource(GRAPHICS->GetShadowMap()->GetComPtr().Get());
	}

    if (_layerMapArraySRV)
    {
        _layerMapArrayEffectBuffer->SetResource(_layerMapArraySRV.get());
    }

	// TODO: 필요할때만 업데이트하기
	_ssaoMapEffectBuffer->SetResource(GRAPHICS->GetSsaoMap()->GetComPtr().Get());
}

shared_ptr<Material> Material::Clone()
{
	shared_ptr<Material> newMat = make_shared<Material>();

	newMat->_desc = _desc;
	newMat->_shader = _shader;
	newMat->_renderQueue = _renderQueue;

	newMat->_diffuseMap = _diffuseMap;
	newMat->_normalMap = _normalMap;
	newMat->_specularMap = _specularMap;
	newMat->_randomTex = _randomTex;
	newMat->_cubeMap = _cubeMap;
    newMat->_layerMapArraySRV = _layerMapArraySRV;

	newMat->_diffuseEffectBuffer = _diffuseEffectBuffer;
	newMat->_normalEffectBuffer = _normalEffectBuffer;
	newMat->_specularEffectBuffer = _specularEffectBuffer;
	newMat->_randomEffectBuffer = _randomEffectBuffer;
	newMat->_cubeMapEffectBuffer = _cubeMapEffectBuffer;
	newMat->_shadowMapEffectBuffer = _shadowMapEffectBuffer;
    newMat->_ssaoMapEffectBuffer = _ssaoMapEffectBuffer;
    newMat->_layerMapArrayEffectBuffer = _layerMapArrayEffectBuffer;

	return newMat;
}
