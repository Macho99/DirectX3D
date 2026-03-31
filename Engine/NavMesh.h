#pragma once
#include "Component.h"
#include "../NavBuild/NavMeshBuilder.h"

class NavMesh : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(NavMesh)
public:
    NavMesh();
    ~NavMesh();

    virtual bool OnGUI() override;

private:
    Vec2 _walkableUV;
    Vec2 _unwalkableUV;

    NavMeshBuilder _builder;
    ComponentRef<MeshRenderer> _walkableMeshRenderer;
};

