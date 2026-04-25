#pragma once
#include "Component.h"
#include "../NavBuild/NavMeshBuilder.h"

#define NAV_DEBUG_LIST \
    X(MarkWalkable) \
    X(BuildHeightField) \
    X(FilterHeightField) \
    X(CompactHeightField) \
    X(BuildContours) \
    X(SimplifyContours) \
    X(BuildPolyMesh) \

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
    bool TryInitializeDebugMesh(NavDebugOption option, bool useMeshRenderer = true);
    void EnsureLineRendererCount(int totalCount);

private:
    NavMeshBuilder _builder;
    ComponentRef<MeshRenderer> _debugMeshRenderer;
    GameObjectRef _debugLineRendererParent;
    vector<ComponentRef<class LineRenderer>> _debugLineRenderers;
    NavDebugOption _debugOption = NavDebugOption::None;
    Vec3 _buildExtent = Vec3(11.f, 20.f, 11.f);
    float _contourSimplifyMaxError = 0.9f;
    bool _debugInvalidTriangle = false;
    int _debugSeedCount = 0;
    bool _showDistanceField = false;
    int _debugPolyIndexCount = 0;

    function<void(const HeightField&)> _heightFieldDebugFunc;
    function<void(const Contours&)> _contoursDebugFunc;
};

