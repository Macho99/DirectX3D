#include "pch.h"
#include "InstancingRenderer.h"
#include "Material.h"
#include "Scene.h"
#include "SceneManager.h"

InstancingRenderer::InstancingRenderer(ComponentType componentType)
    :Super(componentType)
{
}

void InstancingRenderer::LateUpdate()
{
    if (HasInstancingData())
    {
        const Matrix worldMat = GetTransform()->GetWorldMatrix();
        if (worldMat != _lastWorldMatrix || _instDatas.size() != _originInstDatas.size())
        {
            _instDatas.resize(_originInstDatas.size());
            for (size_t i = 0; i < _originInstDatas.size(); i++)
            {
                _instDatas[i] = _originInstDatas[i] * worldMat;
            }
        }
        _lastWorldMatrix = worldMat;
    }
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

void InstancingRenderer::AddInstancingData(const Matrix& mat)
{
    _originInstDatas.push_back(mat);
}
