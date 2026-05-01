#pragma once
#include "Component.h"

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
};