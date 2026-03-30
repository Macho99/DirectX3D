#include "pch.h"
#include "NavMeshBuilder.h"

bool NavMeshBuilder::Build(const NavBuildInput& input, const fs::path& savePath)
{
    Bounds bounds;
    for (const auto& tri : input.triangles)
    {
        bounds.Encapsulate(tri.v0);
        bounds.Encapsulate(tri.v1);
        bounds.Encapsulate(tri.v2);
    }

    return true;
}

bool NavMeshBuilder::MarkWalkableTriangles()
{
    return false;
}
