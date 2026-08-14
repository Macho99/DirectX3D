#include "pch.h"
#include "InstancingRenderer.h"
#include "Material.h"
#include "Scene.h"
#include "SceneManager.h"

InstancingRenderer::InstancingRenderer(ComponentType componentType)
    :Super(componentType)
{
}

void InstancingRenderer::OnMaterialChange(const Material* oldMaterial, const Material* newMaterial)
{
    shared_ptr<Scene> scene = SCENE->GetCurrentScene();
    const GameObjectRef gameObjectRef = GetGameObjectRef();
    if (scene == nullptr || gameObjectRef.IsValid() == false || scene->IsInScene(gameObjectRef) == false)
        return;

    const AssetId meshId = GetMeshId();
    scene->OnInstRendererStateChange(ComponentRef<InstancingRenderer>(this),
        oldMaterial, newMaterial, meshId, meshId);
}

void InstancingRenderer::OnMeshChange(const AssetId& oldMeshId, const AssetId& newMeshId)
{
    if (oldMeshId == newMeshId)
        return;

    shared_ptr<Scene> scene = SCENE->GetCurrentScene();
    const GameObjectRef gameObjectRef = GetGameObjectRef();
    if (scene == nullptr || gameObjectRef.IsValid() == false || scene->IsInScene(gameObjectRef) == false)
        return;

    const Material* material = GetMaterial().Resolve();
    scene->OnInstRendererStateChange(ComponentRef<InstancingRenderer>(this),
        material, material, oldMeshId, newMeshId);
}
