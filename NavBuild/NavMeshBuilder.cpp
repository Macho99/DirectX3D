#include "pch.h"
#include "NavMeshBuilder.h"
#include "Contours.h"

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

    contours.BuildPolyMesh();
    _onBuildPolyMesh(contours);

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

void NavMeshBuilder::MarkWalkableTriangles(NavBuildInput& input)
{
    float walkableThreshold = std::cos(input.settings.agentMaxSlopeDeg * 3.14159265f / 180.f);

    for (int i = 0; i < input.triangles.size(); i++)
    {
        InputTri& tri = input.triangles[i];
        Vec3 n = GetTriangleNormal(tri);
        float dot = Vec3::Dot(n, Vec3(0.f, 1.f, 0.f));
        tri.walkable = dot >= walkableThreshold;
    }
}