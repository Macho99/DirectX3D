#include "pch.h"
#include "Renderer.h"
#include "Material.h"
#include "Camera.h"
#include "Light.h"

Renderer::Renderer(ComponentType componentType) : Super(componentType)
{
}

Renderer::~Renderer()
{
}

// 여기서 검증하고 InnerRender를 호출
bool Renderer::Render(bool isShadowTech)
{
	if (_material == nullptr)
		return false;

	if (_material->GetCastShadow() == false && isShadowTech == true)
		return false;

	InnerRender(isShadowTech);

	return true;
}

void Renderer::InnerRender(bool isShadowTech)
{
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const auto& shader = _material->GetShader();
	if (shader == nullptr)
		return;

	if (isShadowTech == false)
	{
		_material->Update();
		// Light
		auto lightObj = SCENE->GetCurrentScene()->GetLight();
		if (lightObj)
		{
			shader->PushLightData(lightObj->GetLight()->GetLightDesc());
		}
		shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	}
	else
	{
		shader->PushGlobalData(Light::S_MatView, Light::S_MatProjection);
	}

	// GlobalData
	shader->PushTransformData(TransformDesc{ GetTransform()->GetWorldMatrix() });
}
