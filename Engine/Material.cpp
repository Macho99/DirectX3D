#include "pch.h"
#include "Material.h"

Material::Material() : Super(ResourceType::Material)
{
}

Material::~Material()
{
}

void Material::SetShader(ResourceRef<Shader> shader)
{
	_shader = shader;

    Shader* shaderPtr = _shader.Resolve();
	_diffuseEffectBuffer =			shaderPtr->GetSRV("DiffuseMap");
	_normalEffectBuffer =			shaderPtr->GetSRV("NormalMap");
	_specularEffectBuffer =			shaderPtr->GetSRV("SpecularMap");
	_randomEffectBuffer =			shaderPtr->GetSRV("RandomMap");
	_cubeMapEffectBuffer =			shaderPtr->GetSRV("CubeMap");
	_shadowMapEffectBuffer =		shaderPtr->GetSRV("ShadowMap");
	_ssaoMapEffectBuffer =			shaderPtr->GetSRV("SsaoMap");
	_layerMapArrayEffectBuffer =	shaderPtr->GetSRV("LayerMapArray");
}

void Material::SetDiffuseMap(ResourceRef<Texture> diffuseMap)
{
    _diffuseMap = diffuseMap;
}

void Material::SetNormalMap(ResourceRef<Texture> normalMap)
{
    _normalMap = normalMap;
}

void Material::SetSpecularMap(ResourceRef<Texture> specularMap)
{
    _specularMap = specularMap;
}

void Material::SetRandomTex(ResourceRef<Texture> randomTex)
{
    _randomTex = randomTex;
}

void Material::SetCubeMap(ResourceRef<Texture> cubeMap)
{
    _cubeMap = cubeMap;
}

void Material::Update()
{
    Shader* shader = _shader.Resolve();
	if (shader == nullptr)
		return;

	shader->PushMaterialData(_desc);

    Texture* diffuseMap = _diffuseMap.Resolve();
	if (diffuseMap)
	{
		_diffuseEffectBuffer->SetResource(diffuseMap->GetComPtr().Get());
	}

    Texture* normalMap = _normalMap.Resolve();
	if (normalMap)
	{
		_normalEffectBuffer->SetResource(normalMap->GetComPtr().Get());
	}

    Texture* specularMap = _specularMap.Resolve();
	if (specularMap)
	{
		_specularEffectBuffer->SetResource(specularMap->GetComPtr().Get());
	}

    Texture* randomTex = _randomTex.Resolve();
	if (randomTex)
	{
		_randomEffectBuffer->SetResource(randomTex->GetComPtr().Get());
	}
	
    Texture* cubeMap = _cubeMap.Resolve();
	if (cubeMap)
	{
		_cubeMapEffectBuffer->SetResource(cubeMap->GetComPtr().Get());
	}

	if (_castShadow)
	{
		_shadowMapEffectBuffer->SetResource(GRAPHICS->GetShadowArraySRV().Get());
	}

    if (_layerMapArraySRV)
    {
        _layerMapArrayEffectBuffer->SetResource(_layerMapArraySRV.Get());
    }

	// TODO: 필요할때만 업데이트하기
	_ssaoMapEffectBuffer->SetResource(GRAPHICS->GetSsaoMap().Resolve()->GetComPtr().Get());
}

//shared_ptr<Material> Material::Clone()
//{
//	shared_ptr<Material> newMat = make_shared<Material>();
//
//	newMat->_desc = _desc;
//	newMat->_shader = _shader;
//	newMat->_renderQueue = _renderQueue;
//
//	newMat->_diffuseMap = _diffuseMap;
//	newMat->_normalMap = _normalMap;
//	newMat->_specularMap = _specularMap;
//	newMat->_randomTex = _randomTex;
//	newMat->_cubeMap = _cubeMap;
//    newMat->_layerMapArraySRV = _layerMapArraySRV;
//
//	newMat->_diffuseEffectBuffer = _diffuseEffectBuffer;
//	newMat->_normalEffectBuffer = _normalEffectBuffer;
//	newMat->_specularEffectBuffer = _specularEffectBuffer;
//	newMat->_randomEffectBuffer = _randomEffectBuffer;
//	newMat->_cubeMapEffectBuffer = _cubeMapEffectBuffer;
//	newMat->_shadowMapEffectBuffer = _shadowMapEffectBuffer;
//    newMat->_ssaoMapEffectBuffer = _ssaoMapEffectBuffer;
//    newMat->_layerMapArrayEffectBuffer = _layerMapArrayEffectBuffer;
//
//	return newMat;
//}
