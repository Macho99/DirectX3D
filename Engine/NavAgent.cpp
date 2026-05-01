#include "pch.h"
#include "NavAgent.h"
#include "NavMesh.h"
#include "OnGUIUtils.h"
#include "ComponentRef.h"
#include "MeshRenderer.h"
#include "LineRenderer.h"
#include "TessTerrain.h"
#include "../NavBuild/NavMeshQuery.h"
#include "../NavBuild/NavMeshBuilder.h"

NavAgent::NavAgent() : Super(StaticType)
{
}

NavAgent::~NavAgent()
{
}

void NavAgent::Start()
{
    _navMesh = CUR_SCENE->FindComponentRef<NavMesh>();
}

bool NavAgent::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    changed |= OnGUIUtils::DrawComponentRef("NavMesh", _navMesh);
    changed |= OnGUIUtils::DrawComponentRef("Goal Renderer", _goalMeshRenderer);
    changed |= OnGUIUtils::DrawComponentRef("Path Renderer", _pathRenderer);
    changed |= OnGUIUtils::DrawComponentRef("EdgeCenter Path Renderer", _edgeCenterPathRenderer);
    changed |= OnGUIUtils::DrawColor("Debug Color", &_debugColor);

    changed |= OnGUIUtils::DrawEnableButton("Terrain Picking Mode", _terrainPickingMode, true, false);
    if (_terrainPickingMode)
    {
        if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON))
        {
            POINT mousePos = INPUT->GetMousePos();
            TessTerrain* terrain = CUR_SCENE->FindComponentRef<TessTerrain>().Resolve();
            Vec3 worldPos;
            float distance;
            if (TryMakeDebugRenderer() && terrain != nullptr && terrain->Pick(mousePos.x, mousePos.y, worldPos, distance))
            {
                _goalMeshRenderer.Resolve()->GetTransform()->SetPosition(worldPos);
                changed = true;
                FindPath();
            }
        }
    }

    return changed;
}

void NavAgent::FindPath()
{
    if (!TryMakeDebugRenderer())
        return;

    Vec3 startPos = GetTransform()->GetPosition();
    Vec3 goalPos = _goalMeshRenderer.Resolve()->GetTransform()->GetPosition();

    NavPath navPath;
    if (_navMesh.Resolve()->TryFindPath(startPos, goalPos, navPath) == false)
        return;

    LineRenderer* pathRenderer = _pathRenderer.Resolve();
    pathRenderer->ClearPoints();
    for (const Vec3& point : navPath.path)
    {
        pathRenderer->AddPoint(point);
    }

    LineRenderer* edgeCenterPathRenderer = _edgeCenterPathRenderer.Resolve();
    edgeCenterPathRenderer->ClearPoints();
    for (const Vec3& point : navPath.edgeCenterPath)
    {
        edgeCenterPathRenderer->AddPoint(point);
    }
}

bool NavAgent::TryMakeDebugRenderer()
{
    if (_navMesh.Resolve() == nullptr)
        _navMesh = CUR_SCENE->FindComponentRef<NavMesh>();

    if (_navMesh.Resolve() == nullptr)
        return false;

    NavMesh* navMesh = _navMesh.Resolve();
    TransformRef navMeshTransform = navMesh->GetTransformRef();
    string agentName = GetGameObject()->GetName();
    if (_goalMeshRenderer.Resolve() == nullptr)
    {
        GameObject* obj = CUR_SCENE->Add(agentName + " Goal MeshRenderer").Resolve();
        obj->AddComponent(make_unique<MeshRenderer>());
        _goalMeshRenderer = obj->GetFixedComponentRef<MeshRenderer>();
        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\VeigarMaterial.mat");
        auto mesh = RESOURCES->GetSphereMesh();
        obj->GetMeshRenderer()->SetMaterial(materialRef);
        obj->GetMeshRenderer()->SetMesh(mesh);
        obj->GetTransform()->SetParent(navMeshTransform);
    }

    if (_pathRenderer.Resolve() == nullptr)
    {
        GameObject* obj = CUR_SCENE->Add(agentName + " Path LineRenderer").Resolve();
        obj->AddComponent(make_unique<LineRenderer>());
        _pathRenderer = obj->GetFixedComponentRef<LineRenderer>();
        obj->GetTransform()->SetParent(navMeshTransform);
    }

    if (_edgeCenterPathRenderer.Resolve() == nullptr)
    {
        GameObject* obj = CUR_SCENE->Add(agentName + " EdgeCenter Path LineRenderer").Resolve();
        obj->AddComponent(make_unique<LineRenderer>());
        _edgeCenterPathRenderer = obj->GetFixedComponentRef<LineRenderer>();
        obj->GetTransform()->SetParent(navMeshTransform);
    }

    return true;
}
