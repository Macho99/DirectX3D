#include "pch.h"
#include "InstancingRenderer.h"
#include "Material.h"
#include "Scene.h"
#include "SceneManager.h"
#include "EditorManager.h"
#include "SceneView.h"

InstancingRenderer::InstancingRenderer(ComponentType componentType, bool isStatic)
    : Super(componentType), _isStatic(isStatic)
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

    if (_isStatic == false || _boundsInitialized == false)
        UpdateBounds();
}

void InstancingRenderer::OnEnable()
{
    Super::OnEnable();
    InvalidateBounds();
}

bool InstancingRenderer::IsInFrustum(const Vec4 frustumPlanes[6])
{
    if (_boundsInitialized == false)
        UpdateBounds();

    if (_hasWorldBounds == false)
        return true;

    for (uint32 i = 0; i < 6; ++i)
    {
        const Vec4& plane = frustumPlanes[i];
        const float centerDistance =
            plane.x * _worldBounds.Center.x
            + plane.y * _worldBounds.Center.y
            + plane.z * _worldBounds.Center.z
            + plane.w;
        const float projectedRadius =
            fabsf(plane.x) * _worldBounds.Extents.x
            + fabsf(plane.y) * _worldBounds.Extents.y
            + fabsf(plane.z) * _worldBounds.Extents.z;

        if (centerDistance + projectedRadius < 0.0f)
            return false;
    }

    return true;
}

void InstancingRenderer::UpdateBounds()
{
    BoundingBox localBounds;
    if (TryCalculateLocalBounds(OUT localBounds) == false)
    {
        _hasWorldBounds = false;
        _boundsInitialized = true;
        return;
    }

    auto mergeWorldBounds = [this](const BoundingBox& bounds)
        {
            if (_hasWorldBounds == false)
            {
                _worldBounds = bounds;
                _hasWorldBounds = true;
                return;
            }

            const Vec3 currentCenter(_worldBounds.Center);
            const Vec3 currentExtents(_worldBounds.Extents);
            const Vec3 boundsCenter(bounds.Center);
            const Vec3 boundsExtents(bounds.Extents);
            const Vec3 currentMin = currentCenter - currentExtents;
            const Vec3 currentMax = currentCenter + currentExtents;
            const Vec3 boundsMin = boundsCenter - boundsExtents;
            const Vec3 boundsMax = boundsCenter + boundsExtents;
            const Vec3 mergedMin = Vec3::Min(currentMin, boundsMin);
            const Vec3 mergedMax = Vec3::Max(currentMax, boundsMax);
            _worldBounds.Center = (mergedMin + mergedMax) * 0.5f;
            _worldBounds.Extents = (mergedMax - mergedMin) * 0.5f;
        };

    _hasWorldBounds = false;
    if (HasInstancingData())
    {
        for (const InstancingData& instancingData : _instDatas)
        {
            BoundingBox instanceBounds;
            localBounds.Transform(instanceBounds, instancingData);
            mergeWorldBounds(instanceBounds);
        }
    }
    else
    {
        BoundingBox transformedBounds;
        localBounds.Transform(transformedBounds, GetTransform()->GetWorldMatrix());
        mergeWorldBounds(transformedBounds);
    }

    _boundsInitialized = true;
}

void InstancingRenderer::OnMaterialChange(const Material* oldMaterial, const Material* newMaterial)
{
    Scene* scene = SCENE->GetCurrentScene();
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

    Scene* scene = SCENE->GetCurrentScene();
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
    InvalidateBounds();
}

bool InstancingRenderer::OnGUI()
{
    bool changed = Super::OnGUI();

    ImGui::SeparatorText("Instancing Data");
    ImGui::Text("Count: %zu", _instDatas.size());

    SceneView* sceneView = EDITOR->GetSceneView();
    const bool selectionOutOfRange = _selectedInstDataIndex >= static_cast<int>(_instDatas.size());
    if (selectionOutOfRange)
    {
        _selectedInstDataIndex = -1;
        if (sceneView != nullptr)
            sceneView->ClearTransformGizmoOverride();
    }

    if (_instDatas.empty())
    {
        ImGui::TextDisabled("No instancing data.");
        return changed;
    }

    float maxHeightCount = min(8.f, static_cast<float>(_instDatas.size()));
    const float listHeight = ImGui::GetTextLineHeightWithSpacing() * maxHeightCount + 12;
    if (ImGui::BeginChild("##InstancingDataList", ImVec2(0.0f, listHeight),
        ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(_instDatas.size()));
        while (clipper.Step())
        {
            for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
            {
                const Vec3 position = _instDatas[index].Translation();
                const string label = Utils::Format("[%d]  (%.3f, %.3f, %.3f)",
                    index, position.x, position.y, position.z);
                if (ImGui::Selectable(label.c_str(), _selectedInstDataIndex == index))
                    _selectedInstDataIndex = index;
            }
        }
    }
    ImGui::EndChild();

    if (_selectedInstDataIndex < 0)
        return changed;

    const size_t selectedIndex = static_cast<size_t>(_selectedInstDataIndex);
    auto applyWorldMatrix = [this, selectedIndex](const Matrix& worldMatrix)
        {
            if (selectedIndex >= _originInstDatas.size())
                return;

            const Matrix rendererWorldInv = GetTransform()->GetWorldMatrix().Invert();
            _originInstDatas[selectedIndex] = worldMatrix * rendererWorldInv;

            if (selectedIndex < _instDatas.size())
                _instDatas[selectedIndex] = worldMatrix;

            InvalidateBounds();
        };

    Matrix selectedWorld = _instDatas[selectedIndex];
    Vec3 worldPosition = selectedWorld.Translation();
    if (ImGui::DragFloat3("World Position", &worldPosition.x, 0.1f))
    {
        selectedWorld.Translation(worldPosition);
        applyWorldMatrix(selectedWorld);
        changed = true;
    }

    if (ImGui::Button("Clear Selection"))
    {
        _selectedInstDataIndex = -1;
        if (sceneView != nullptr)
            sceneView->ClearTransformGizmoOverride();
    }
    else if (sceneView != nullptr)
    {
        sceneView->SetTransformGizmoOverride(selectedWorld, std::move(applyWorldMatrix));
    }

    return changed;
}

void InstancingRenderer::OnInspectorFocusLost()
{
    Super::OnInspectorFocusLost();
    _selectedInstDataIndex = -1;

    SceneView* sceneView = EDITOR->GetSceneView();
    if (sceneView != nullptr)
        sceneView->ClearTransformGizmoOverride();
}
