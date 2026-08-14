#include "pch.h"
#include "Renderer.h"
#include "Material.h"
#include "Camera.h"
#include "Light.h"
#include "OnGUIUtils.h"
#include "Scene.h"
#include "SceneManager.h"

Renderer::Renderer(ComponentType componentType) : Super(componentType)
{
}

Renderer::~Renderer()
{
}

void Renderer::SetMaterial(ResourceRef<Material> material)
{
    if (_material == material)
        return;

    const Material* oldMaterial = _material.Resolve();
    _material = material;
    OnMaterialChange(oldMaterial, _material.Resolve());
}

// 여기서 검증하고 InnerRender를 호출
bool Renderer::Render(RenderTech renderTech)
{
    if (CanRender(renderTech) == false)
        return false;

	InnerRender(renderTech);
	return true;
}

bool Renderer::CanRender(RenderTech renderTech)
{
	if (_material.IsValid() == false)
		return false;

	if (_material.Resolve()->GetCastShadow() == false && renderTech == RenderTech::Shadow)
		return false;

	if (_material.Resolve()->GetShader()->CanDraw(renderTech) == false)
		return false;

	return true;
}

bool Renderer::OnGUI()
{
	bool changed = false;

	changed |= Super::OnGUI();

    changed |= OnGUIUtils::DrawUInt8("Pass", & _pass, 1.f);
    ResourceRef<Material> material = GetMaterial();
    if (OnGUIUtils::DrawResourceRef("Material", material))
    {
        SetMaterial(material);
        changed = true;
    }

	return changed;
}

bool Renderer::IsInstRenderer() const
{
	switch (GetType())
	{
    case ComponentType::MeshRenderer:
    case ComponentType::ModelRenderer:
    case ComponentType::ModelAnimator:
		return true;
	}

	return false;
}

void Renderer::OnMaterialChange(const Material* oldMaterial, const Material* newMaterial)
{
    shared_ptr<Scene> scene = SCENE->GetCurrentScene();
    const GameObjectRef gameObjectRef = GetGameObjectRef();
    if (scene == nullptr || gameObjectRef.IsValid() == false || scene->IsInScene(gameObjectRef) == false)
        return;

    scene->OnRendererMaterialChange(ComponentRef<Renderer>(this), oldMaterial, newMaterial);
}

void Renderer::InnerRender(RenderTech renderTech)
{
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Material* material = _material.Resolve();
	const auto& shader = material->GetShader();
	if (shader == nullptr)
		return;

	if (_beforeRender)
		_beforeRender(material);


	if (renderTech != RenderTech::Shadow)
	{
		material->Update();
		// Light
		auto lightObj = SCENE->GetCurrentScene()->GetLight();
		if (lightObj)
		{
			shader->PushLightData(lightObj->GetLight()->GetLightDesc());
		}
		shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
		shader->PushShadowData(Light::S_ShadowData);
	}
	else
	{
		shader->PushGlobalData(Light::S_MatView, Light::S_MatProjection);
	}

	// GlobalData
	shader->PushTransformData(TransformDesc{ GetTransform()->GetWorldMatrix() });
}
