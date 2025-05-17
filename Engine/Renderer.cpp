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

void Renderer::Render()
{
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	if (_material == nullptr)
		return;
	_material->Update();

	const auto& shader = _material->GetShader();
	if (shader == nullptr)
		return;

	// GlobalData
	shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	shader->PushTransformData(TransformDesc{ GetTransform()->GetWorldMatrix() });

	// Light
	auto lightObj = SCENE->GetCurrentScene()->GetLight();
	if (lightObj)
	{
		shader->PushLightData(lightObj->GetLight()->GetLightDesc());
	}
}
