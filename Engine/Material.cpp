#include "pch.h"
#include "Material.h"
#include "OnGUIUtils.h"

Material::Material() : Super(StaticType)
{
}

Material::~Material()
{
}

void Material::SetShader(ResourceRef<Shader> shader)
{
	_shader = shader;
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

void Material::SetCubeMap(ResourceRef<Texture> cubeMap)
{
    _cubeMap = cubeMap;
}

void Material::Update()
{
    Shader* shader = _shader.Resolve();
	if (shader == nullptr)
		return;

	InitializeEffectBuffers();

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

	if (_useRandomTexture)
	{
		_randomEffectBuffer->SetResource(RESOURCES->GetRandomTexture()->GetComPtr().Get());
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

bool Material::OnGUI(bool isReadOnly)
{
    Super::OnGUI(isReadOnly);

	//ar(CEREAL_NVP(_desc));
	//ar(CEREAL_NVP(_renderQueue));
	//ar(CEREAL_NVP(_castShadow));
	//ar(CEREAL_NVP(_shader));
	//ar(CEREAL_NVP(_diffuseMap));
	//ar(CEREAL_NVP(_normalMap));
	//ar(CEREAL_NVP(_specularMap));
	//ar(CEREAL_NVP(_randomTex));
	//ar(CEREAL_NVP(_cubeMap));
	bool changed = false;
    changed |= OnGUIUtils::DrawInt32("Version", &_version, 1.f, true);
	changed |= OnGUIUtils::DrawColor("Ambient", &_desc.ambient, isReadOnly);
	changed |= OnGUIUtils::DrawColor("Diffuse", &_desc.diffuse, isReadOnly);
	changed |= OnGUIUtils::DrawColor("Specular", &_desc.specular, isReadOnly);
	changed |= OnGUIUtils::DrawColor("Emissive", &_desc.emissive, isReadOnly);
    changed |= OnGUIUtils::DrawEnumCombo("RenderQueue", _renderQueue, RenderQueueNames, (int)RenderQueue::Max, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("Shader", _shader, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("DiffuseMap", _diffuseMap, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("NormalMap", _normalMap, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("SpecularMap", _specularMap, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("CubeMap", _cubeMap, isReadOnly);
	changed |= OnGUIUtils::DrawBool("UseRandomTexture", &_useRandomTexture, isReadOnly);
    changed |= OnGUIUtils::DrawBool("CastShadow", &_castShadow, isReadOnly);
    changed |= OnGUIUtils::DrawBool("IncludeInNavMesh", &_includeInNavMesh, isReadOnly);

	return changed;
}

void Material::InitializeEffectBuffers()
{
    if (_initializedEffectBuffers)
        return;

	Shader* shaderPtr = _shader.Resolve();
    if (shaderPtr == nullptr)
        return;

	if (shaderPtr == nullptr)
	{
		_diffuseEffectBuffer = nullptr;
		_normalEffectBuffer = nullptr;
		_specularEffectBuffer = nullptr;
		_randomEffectBuffer = nullptr;
		_cubeMapEffectBuffer = nullptr;
		_shadowMapEffectBuffer = nullptr;
		_ssaoMapEffectBuffer = nullptr;
		_layerMapArrayEffectBuffer = nullptr;
		return;
	}

	_diffuseEffectBuffer = shaderPtr->GetSRV("DiffuseMap").Get();
	_normalEffectBuffer = shaderPtr->GetSRV("NormalMap").Get();
	_specularEffectBuffer = shaderPtr->GetSRV("SpecularMap").Get();
	_randomEffectBuffer = shaderPtr->GetSRV("RandomMap").Get();
	_cubeMapEffectBuffer = shaderPtr->GetSRV("CubeMap").Get();
	_shadowMapEffectBuffer = shaderPtr->GetSRV("ShadowMap").Get();
	_ssaoMapEffectBuffer = shaderPtr->GetSRV("SsaoMap").Get();
	_layerMapArrayEffectBuffer = shaderPtr->GetSRV("LayerMapArray").Get();

    _initializedEffectBuffers = true;
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
