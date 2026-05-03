#pragma once
#include "Component.h"
#include "../NavBuild/Types.h"

class NavMesh;
class MeshRenderer;
class LineRenderer;

class NavAgent : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(NavAgent)
public:
    NavAgent();
    ~NavAgent();

    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    void FindPath();

private:
    bool TryMakeDebugRenderer();

private:
    Color _debugColor = Colors::Yellow;

private:
    ComponentRef<NavMesh> _navMesh;
    ComponentRef<MeshRenderer> _goalMeshRenderer;
    ComponentRef<LineRenderer> _pathRenderer;
    ComponentRef<LineRenderer> _edgeCenterPathRenderer;
    bool _terrainPickingMode = false;

    MoveConfig _moveConfig;
    MoveInfo _moveInfo;
    float _offsetY = 180.f;
};