#include "pch.h"
#include "InstancingRenderer.h"
#include "Material.h"
#include "Scene.h"
#include "SceneManager.h"
#include "EditorManager.h"
#include "SceneView.h"

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
