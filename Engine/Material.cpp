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
}

shared_ptr<Material> Material::Clone()
{
	shared_ptr<Material> newMat = make_shared<Material>();

	newMat->_desc = _desc;
	newMat->_shader = _shader;
	newMat->_diffuseMap = _diffuseMap;
	newMat->_normalMap = _normalMap;
	newMat->_specularMap = _specularMap;
	newMat->_diffuseEffectBuffer = _diffuseEffectBuffer;
	newMat->_normalEffectBuffer = _normalEffectBuffer;
	newMat->_specularEffectBuffer = _specularEffectBuffer;

	return newMat;
}
