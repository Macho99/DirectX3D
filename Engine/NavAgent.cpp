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
    _navMesh = CUR_SCENE->FindComponent<NavMesh>();
}

void NavAgent::Update()
{
    if (TryMakeDebugRenderer() == false)
        return;

    Transform* transform = GetTransform();
    const Vec3 position = transform->GetPosition();
    _moveInfo.position = position;
    Vec3 rotation = GetTransform()->GetRotation();
    rotation.y -= _offsetY;
    _moveInfo.rotationY = rotation.y;
    NavMesh* navMesh = _navMesh.Resolve();
    if (navMesh->MoveAlongPath(_moveConfig, _moveInfo, TIME->GetDeltaTime()) == false)
    {
        return;
    }

    transform->SetPosition(_moveInfo.position);
    rotation.y = _moveInfo.rotationY;
    rotation.y += _offsetY;
    transform->SetRotation(rotation);
}

void NavAgent::LateUpdate()
{
    NavMesh* navMesh = _navMesh.Resolve();
    if (navMesh == nullptr)
        return;

    Transform* transform = GetTransform();
    const Vec3 position = transform->GetPosition();
    _validatePositionInfo.curPosition = position;
    bool needToRemap = navMesh->ValidatePosition(_validatePositionInfo);

    if (needToRemap)
    {
        transform->SetPosition(_validatePositionInfo.validatedPosition);
    }
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
    changed |= OnGUIUtils::DrawFloat("speed", &_moveConfig.speed, 0.1f);
    changed |= OnGUIUtils::DrawFloat("turnSpeed", &_moveConfig.turnSpeed, 0.1f);

    changed |= OnGUIUtils::DrawEnableButton("Terrain Picking Mode", _terrainPickingMode, true, false);
    if (_terrainPickingMode)
    {
        if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON))
        {
            POINT mousePos = INPUT->GetMousePos();
            TessTerrain* terrain = CUR_SCENE->FindComponent<TessTerrain>();
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
    if (ImGui::Button("Find Path"))
        FindPath();

    return changed;
}

void NavAgent::FindPath()
{
    if (!TryMakeDebugRenderer())
        return;

    Vec3 startPos = GetTransform()->GetPosition();
    Vec3 goalPos = _goalMeshRenderer.Resolve()->GetTransform()->GetPosition();

    if (_navMesh.Resolve()->TryFindPath(startPos, goalPos, _moveInfo) == false)
        return;

    NavPath& navPath = _moveInfo.navPath;
    LineRenderer* pathRenderer = _pathRenderer.Resolve();
    pathRenderer->SetColor(Colors::Green);
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
        _navMesh = CUR_SCENE->FindComponent<NavMesh>();

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
