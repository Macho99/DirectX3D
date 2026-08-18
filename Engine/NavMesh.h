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
    X(BuildDetailMesh) \

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

    virtual void Awake() override;
    virtual void Start() override;
    virtual bool OnGUI() override;

    bool TryFindPath(const Vec3& worldStart, const Vec3& worldEnd, MoveInfo& moveInfo) const;
    bool MoveAlongPath(const MoveConfig& config, MoveInfo& moveInfo, float deltaTime) const
    {
        if (_builder.IsBuilt() == false)
            return false;
        return _builder.MoveAlongPath(config, moveInfo, deltaTime);
    }
    bool ValidatePosition(ValidatePositionInfo& info) const
    {
        if (_builder.IsBuilt() == false)
            return false;
        return _builder.ValidatePosition(info);
    }

private:
    Vec3 GetDebugColor(int id);
    bool TryInitializeDebugMesh(NavDebugOption option, bool useMeshRenderer = true);
    void EnsureLineRendererCount(int totalCount);

private:
    NavMeshBuilder _builder;
    ComponentRef<MeshRenderer> _debugMeshRenderer;
    GameObjectRef _debugLineRendererParent;
    vector<ComponentRef<class LineRenderer>> _debugLineRenderers;
    //ComponentRef<Transform> _startPoint;
    //ComponentRef<Transform> _endPoint;

    NavDebugOption _debugOption = NavDebugOption::None;
    Vec3 _buildExtent = Vec3(11.f, 100.f, 11.f);
    bool _showDistanceField = false;
    int _debugPolyIndexCount = 0;
    NavBuildSettings _buildSettings;

    function<void(const HeightField&)> _heightFieldDebugFunc;
    function<void(const Contours&)> _contoursDebugFunc;
};

