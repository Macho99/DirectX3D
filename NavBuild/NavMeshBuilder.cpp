#include "pch.h"
#include "NavMeshBuilder.h"

bool NavMeshBuilder::Build(NavBuildInput input, const fs::path& savePath)
{
    Bounds bounds;

    for (auto& tri : input.triangles)
    {
        bounds.Encapsulate(tri.v0);
        bounds.Encapsulate(tri.v1);
        bounds.Encapsulate(tri.v2);
    }

    if (MarkWalkableTriangles(input) == false)
        return false;

    HeightField heightfield(bounds, input.settings.cellSize, input.settings.cellHeight);
    heightfield.HandleTriangles(input.triangles);
    _onBuildHeightField(heightfield);

    return true;
}

Vec3 NavMeshBuilder::GetTriangleNormal(const InputTri& tri)
{
    Vec3 e0 = tri.v1 - tri.v0;
    Vec3 e1 = tri.v2 - tri.v0;
    Vec3 n = Vec3::Cross(e0, e1);
    n = Vec3::Normalize(n);
    return n;
}

bool NavMeshBuilder::MarkWalkableTriangles(NavBuildInput& input)
{
    float walkableThreshold = std::cos(input.settings.agentMaxSlopeDeg * 3.14159265f / 180.f);

    for (auto& tri : input.triangles)
    {
        Vec3 n = GetTriangleNormal(tri);
        tri.walkable = Vec3::Dot(n, Vec3(0.f, 1.f, 0.f)) >= walkableThreshold;
    }

    if(_onMarkWalkableTriangles)
        _onMarkWalkableTriangles(input.triangles);

    return true;
}