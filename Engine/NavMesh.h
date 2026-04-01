#pragma once
#include "Component.h"
#include "../NavBuild/NavMeshBuilder.h"

#define NAV_DEBUG_LIST \
    X(MarkWalkable) \
    X(BuildHeightField) \
    X(FilterHeightField) \
    X(CompactHeightField) \

enum class NavDebugOption
{
    None,
#define X(name, func) name,
    NAV_DEBUG_LIST
#undef X
    Max
};

static const char* NavDebugNames[] =
{
    "None",
#define X(name, func) #name,
    NAV_DEBUG_LIST
#undef X
};


class NavMesh : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(NavMesh)
public:
    NavMesh();
    ~NavMesh();

    virtual bool OnGUI() override;

private:
    Vec3 GetDebugColor(int id);

private:
    NavMeshBuilder _builder;
    ComponentRef<MeshRenderer> _debugMeshRenderer;

    NavDebugOption _debugOption = NavDebugOption::None;
    Vec3 _buildExtent = Vec3(10.f, 10.f, 10.f);

    function<void(const HeightField&)> _heightFieldDebugFunc;
};

