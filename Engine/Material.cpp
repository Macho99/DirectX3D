#include "pch.h"
#include "Material.h"
#include "OnGUIUtils.h"
#include "Camera.h"

Material::Material() : Super(StaticType)
{
}

Material::~Material()
{
}

void Material::SetShader(ResourceRef<Shader> shader)
{
	_shader = shader;
	_initializedEffectBuffers = false;
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

	SyncShaderProperties();
	InitializeEffectBuffers();

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
	else
	{
		_normalEffectBuffer->SetResource(RESOURCES->GetDummyTexture().Resolve()->GetComPtr().Get());
	}

	shader->PushMaterialData(_desc);
	shader->ApplyMaterialProperties(_shaderProperties);

    Texture* specularMap = _specularMap.Resolve();
	if (specularMap)
	{
		_specularEffectBuffer->SetResource(specularMap->GetComPtr().Get());
	}
	else
	{
        _specularEffectBuffer->SetResource(RESOURCES->GetDummyTexture().Resolve()->GetComPtr().Get());
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

    if (_layerNormalMapArraySRV)
    {
        _layerNormalMapArrayEffectBuffer->SetResource(_layerNormalMapArraySRV.Get());
    }

    if (_layerHeightMapArraySRV)
    {
        _layerHeightMapArrayEffectBuffer->SetResource(_layerHeightMapArraySRV.Get());
    }

	// TODO: 필요할때만 업데이트하기
	if(Camera::S_ProjectionType == ProjectionType::Perspective)
		_ssaoMapEffectBuffer->SetResource(GRAPHICS->GetSsaoMap().Resolve()->GetComPtr().Get());
	else
        _ssaoMapEffectBuffer->SetResource(RESOURCES->GetDummyTexture().Resolve()->GetComPtr().Get());
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

	SyncShaderProperties();
	Shader* shader = _shader.Resolve();
	if (shader && _shaderProperties.empty() == false)
	{
		ImGui::Separator();
		ImGui::TextUnformatted("Shader Properties");

		const vector<ShaderPropertyDesc>& propertyDescs = shader->GetMaterialProperties();
		ImGui::SameLine();
		if (isReadOnly)
			ImGui::BeginDisabled();

		if (ImGui::Button("Reset"))
		{
			for (size_t i = 0; i < propertyDescs.size(); i++)
			{
				_shaderProperties[i].float4Value = propertyDescs[i].defaultFloat4Value;
				_shaderProperties[i].intValue = propertyDescs[i].defaultIntValue;
				_shaderProperties[i].boolValue = propertyDescs[i].defaultBoolValue;
				_shaderProperties[i].textureValue = ResourceRef<Texture>();
				_shaderProperties[i].matrixValue = propertyDescs[i].defaultMatrixValue;
			}
			changed = true;
		}
		if (isReadOnly)
			ImGui::EndDisabled();

		for (size_t i = 0; i < propertyDescs.size(); i++)
		{
			const ShaderPropertyDesc& desc = propertyDescs[i];
			MaterialPropertyValue& property = _shaderProperties[i];

			if (desc.type == ShaderPropertyType::Color)
			{
				changed |= OnGUIUtils::DrawColor(desc.displayName.c_str(), &property.float4Value, isReadOnly);
				continue;
			}

			if (desc.type == ShaderPropertyType::Bool)
			{
				changed |= OnGUIUtils::DrawBool(desc.displayName.c_str(), &property.boolValue, isReadOnly);
				continue;
			}

			if (desc.type == ShaderPropertyType::Texture2D)
			{
				changed |= OnGUIUtils::DrawResourceRef(desc.displayName.c_str(), property.textureValue, isReadOnly);
				continue;
			}

			if (desc.type == ShaderPropertyType::Float2)
			{
				Vec2 value(property.float4Value.x, property.float4Value.y);
				if (OnGUIUtils::DrawVec2(desc.displayName.c_str(), &value, 0.1f, isReadOnly))
				{
					property.float4Value.x = value.x;
					property.float4Value.y = value.y;
					changed = true;
				}
				continue;
			}

			if (desc.type == ShaderPropertyType::Float3)
			{
				Vec3 value(property.float4Value.x, property.float4Value.y, property.float4Value.z);
				if (OnGUIUtils::DrawVec3(desc.displayName.c_str(), &value, 0.1f, isReadOnly))
				{
					property.float4Value.x = value.x;
					property.float4Value.y = value.y;
					property.float4Value.z = value.z;
					changed = true;
				}
				continue;
			}

			if (desc.type == ShaderPropertyType::Float4)
			{
				ImGui::PushID(desc.name.c_str());
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(desc.displayName.c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(200.f);
				if (isReadOnly)
					ImGui::BeginDisabled();
				changed |= ImGui::DragFloat4("##value", &property.float4Value.x, 0.1f);
				if (isReadOnly)
					ImGui::EndDisabled();
				ImGui::PopID();
				continue;
			}

			if (desc.type == ShaderPropertyType::Matrix)
			{
				ImGui::PushID(desc.name.c_str());
				if (isReadOnly)
					ImGui::BeginDisabled();
				ImGui::TextUnformatted(desc.displayName.c_str());
				ImGui::Indent();
				changed |= ImGui::DragFloat4("Row 0", &property.matrixValue._11, 0.1f);
				changed |= ImGui::DragFloat4("Row 1", &property.matrixValue._21, 0.1f);
				changed |= ImGui::DragFloat4("Row 2", &property.matrixValue._31, 0.1f);
				changed |= ImGui::DragFloat4("Row 3", &property.matrixValue._41, 0.1f);
				ImGui::Unindent();
				if (isReadOnly)
					ImGui::EndDisabled();
				ImGui::PopID();
				continue;
			}

			if (desc.type == ShaderPropertyType::Int)
			{
				if (desc.hasRange == false)
				{
					changed |= OnGUIUtils::DrawInt32(desc.displayName.c_str(), &property.intValue, 1.f, isReadOnly);
					continue;
				}

				ImGui::PushID(desc.name.c_str());
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(desc.displayName.c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(200.f);
				if (isReadOnly)
					ImGui::BeginDisabled();
				changed |= ImGui::SliderInt("##value", &property.intValue, desc.minIntValue, desc.maxIntValue);
				if (isReadOnly)
					ImGui::EndDisabled();
				ImGui::PopID();
				continue;
			}

			if (desc.hasRange == false)
			{
				changed |= OnGUIUtils::DrawFloat(desc.displayName.c_str(), &property.float4Value.x, 0.1f, isReadOnly);
				continue;
			}

			ImGui::PushID(desc.name.c_str());
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(desc.displayName.c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(200.f);
			if (isReadOnly)
				ImGui::BeginDisabled();
			changed |= ImGui::SliderFloat("##value", &property.float4Value.x, desc.minValue, desc.maxValue);
			if (isReadOnly)
				ImGui::EndDisabled();
			ImGui::PopID();
		}
		ImGui::Separator();
	}

	changed |= OnGUIUtils::DrawResourceRef("DiffuseMap", _diffuseMap, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("NormalMap", _normalMap, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("SpecularMap", _specularMap, isReadOnly);
	changed |= OnGUIUtils::DrawResourceRef("CubeMap", _cubeMap, isReadOnly);
    changed |= OnGUIUtils::DrawBool("UseRandomTexture", &_useRandomTexture, isReadOnly);
    changed |= OnGUIUtils::DrawBool("CastShadow", &_castShadow, isReadOnly);
    changed |= OnGUIUtils::DrawBool("DrawNormalDepth", &_drawNormalDepth, isReadOnly);
    changed |= OnGUIUtils::DrawBool("IncludeInNavMesh", &_includeInNavMesh, isReadOnly);

	if (shader)
	{
		static const char* renderTechNames[] =
		{
			"Shadow TechNum",
			"Draw TechNum",
			"NormalDepth TechNum",
			"Distortion TechNum",
		};

		ImGui::Separator();
		ImGui::TextUnformatted("Shader TechNums");

		const int maxTechNum = static_cast<int>(shader->GetTechniques().size()) - 1;
		for (int i = 0; i < static_cast<int>(RenderTech::Max); i++)
		{
			const RenderTech renderTech = static_cast<RenderTech>(i);
			int techNum = shader->GetTechNum(renderTech);
			if (OnGUIUtils::DrawInt32(renderTechNames[i], &techNum, 1.f, isReadOnly))
			{
				techNum = std::clamp(techNum, -1, maxTechNum);
				shader->SetTechNum(renderTech, techNum);
				changed = true;
			}
		}
	}

	return changed;
}

void Material::Clone(Material* other)
{
    _desc = other->_desc;
    _renderQueue = other->_renderQueue;
    _castShadow = other->_castShadow;
    _drawNormalDepth = other->_drawNormalDepth;
    _includeInNavMesh = other->_includeInNavMesh;
	_shader = other->_shader;
	_shaderProperties = other->_shaderProperties;
    _diffuseMap = other->_diffuseMap;
    _normalMap = other->_normalMap;
    _specularMap = other->_specularMap;
    _cubeMap = other->_cubeMap;
    _layerMapArraySRV = other->_layerMapArraySRV;
    _layerNormalMapArraySRV = other->_layerNormalMapArraySRV;
    _layerHeightMapArraySRV = other->_layerHeightMapArraySRV;
    _diffuseEffectBuffer = other->_diffuseEffectBuffer;
    _normalEffectBuffer = other->_normalEffectBuffer;
    _specularEffectBuffer = other->_specularEffectBuffer;
    _randomEffectBuffer = other->_randomEffectBuffer;
    _cubeMapEffectBuffer = other->_cubeMapEffectBuffer;
    _shadowMapEffectBuffer = other->_shadowMapEffectBuffer;
    _ssaoMapEffectBuffer = other->_ssaoMapEffectBuffer;
    _layerMapArrayEffectBuffer = other->_layerMapArrayEffectBuffer;
    _layerNormalMapArrayEffectBuffer = other->_layerNormalMapArrayEffectBuffer;
    _layerHeightMapArrayEffectBuffer = other->_layerHeightMapArrayEffectBuffer;
	_initializedEffectBuffers = other->_initializedEffectBuffers;
}

void Material::SyncShaderProperties()
{
	Shader* shader = _shader.Resolve();
	if (shader == nullptr)
		return;

	const vector<ShaderPropertyDesc>& descs = shader->GetMaterialProperties();
	bool isSynchronized = _shaderProperties.size() == descs.size();
	if (isSynchronized)
	{
		for (size_t i = 0; i < descs.size(); i++)
		{
			if (_shaderProperties[i].name != descs[i].name || _shaderProperties[i].type != descs[i].type)
			{
				isSynchronized = false;
				break;
			}
		}
	}

	if (isSynchronized)
		return;

	vector<MaterialPropertyValue> synchronizedProperties;
	synchronizedProperties.reserve(descs.size());
	for (const ShaderPropertyDesc& desc : descs)
	{
		auto iter = find_if(_shaderProperties.begin(), _shaderProperties.end(), [&desc](const MaterialPropertyValue& property)
		{
			return property.name == desc.name && property.type == desc.type;
		});

		MaterialPropertyValue property;
		property.name = desc.name;
		property.type = desc.type;
		if (iter != _shaderProperties.end())
		{
			property.float4Value = iter->float4Value;
			property.intValue = iter->intValue;
			property.boolValue = iter->boolValue;
			property.textureValue = iter->textureValue;
			property.matrixValue = iter->matrixValue;
		}
		else
		{
			property.float4Value = desc.defaultFloat4Value;
			property.intValue = desc.defaultIntValue;
			property.boolValue = desc.defaultBoolValue;
			property.matrixValue = desc.defaultMatrixValue;
		}
		synchronizedProperties.push_back(property);
	}

	_shaderProperties = move(synchronizedProperties);
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
		_layerNormalMapArrayEffectBuffer = nullptr;
		_layerHeightMapArrayEffectBuffer = nullptr;
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
	_layerNormalMapArrayEffectBuffer = shaderPtr->GetSRV("LayerNormalMapArray").Get();
	_layerHeightMapArrayEffectBuffer = shaderPtr->GetSRV("LayerHeightMapArray").Get();

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
