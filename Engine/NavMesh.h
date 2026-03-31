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
    NavMeshBuilder _builder;
    ComponentRef<MeshRenderer> _walkableMeshRenderer;
};

