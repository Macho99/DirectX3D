#include "pch.h"
#include "NavMeshBuilder.h"
#include "Contours.h"
#include "PolyMeshField.h"
#include "DetailMeshField.h"
#include "NavMeshQuery.h"

bool NavMeshBuilder::Build(NavBuildInput input, const fs::path& savePath)
{
    Bounds bounds;
    NavBuildSettings& settings = input.settings;
    for (auto& tri : input.triangles)
    {
        bounds.Encapsulate(tri.v0);
        bounds.Encapsulate(tri.v1);
        bounds.Encapsulate(tri.v2);
    }

    MarkWalkableTriangles(input);
    _onMarkWalkableTriangles(input.triangles);

    HeightField heightfield(bounds, input.settings.cellSize, input.settings.cellHeight);
    heightfield.HandleTriangles(input.triangles);
    _onBuildHeightField(heightfield);

    heightfield.FilterWalkable(input.settings.agentHeight, input.settings.agentMaxClimb);
    _onFilterHeightField(heightfield);

    CompactHeightField compactHeightField(heightfield, settings);
    _onCompactHeightField(compactHeightField);

    Contours contours(compactHeightField, input.settings);
    _onBuildContours(contours);

    contours.RDPSimplify(input.settings.contourMaxError);
    _onSimplifyContours(contours);

    _polyMeshField = make_unique<PolyMeshField>(contours);
    _onBuildPolyMesh(*_polyMeshField);

    _detailMeshField = make_unique<DetailMeshField>(*_polyMeshField, compactHeightField, input.settings);
    _onBuildDetailMesh(*_detailMeshField);

    _navMeshQuery = make_unique<NavMeshQuery>(*_polyMeshField, *_detailMeshField);

    return true;
}

bool NavMeshBuilder::TryFindPath(const Vec3& worldStart, const Vec3& worldEnd, MoveInfo& moveInfo) const
{
    if (_navMeshQuery == nullptr)
        return false;

    moveInfo.Init();
    NavPath& navPath = moveInfo.navPath;

    const Vec3 navStart = _polyMeshField->ToNavPos(worldStart);
    const Vec3 navEnd = _polyMeshField->ToNavPos(worldEnd);

    bool result = _navMeshQuery->TryFindPath(navStart, navEnd, moveInfo);
    if (result == false)
        return false;

    vector<Vec3>& path = navPath.path;
    for (int i = 0; i < path.size(); i++)
    {
        path[i] = _polyMeshField->ToWorldPos(path[i]);
    }
    vector<Vec3>& edgePath = navPath.edgeCenterPath;
    for (int i = 0; i < edgePath.size(); i++)
    {
        edgePath[i] = _polyMeshField->ToWorldPos(edgePath[i]);
    }

    moveInfo.state = MoveInfo::State::Moving;
    return true;
}

bool NavMeshBuilder::MoveAlongPath(const MoveConfig& config, MoveInfo& moveInfo, float deltaTime) const
{
    if (_navMeshQuery == nullptr)
        return false;
    return _navMeshQuery->MoveAlongPath(config, moveInfo, deltaTime);
}
Vec3 NavMeshBuilder::GetTriangleNormal(const InputTri& tri)
{
    Vec3 e0 = tri.v1 - tri.v0;
    Vec3 e1 = tri.v2 - tri.v0;
    Vec3 n = e0.Cross(e1);
    n.Normalize();
    return n;
}

void NavMeshBuilder::MarkWalkableTriangles(NavBuildInput& input)
{
    float walkableThreshold = std::cos(input.settings.agentMaxSlopeDeg * 3.14159265f / 180.f);

    for (int i = 0; i < input.triangles.size(); i++)
    {
        InputTri& tri = input.triangles[i];
        Vec3 n = GetTriangleNormal(tri);
        float dot = n.Dot(Vec3(0.f, 1.f, 0.f));
        tri.walkable = dot >= walkableThreshold;
    }
}