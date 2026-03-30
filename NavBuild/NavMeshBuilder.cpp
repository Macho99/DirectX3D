#include "pch.h"
#include "NavMeshBuilder.h"

int NavMeshBuilder::Build(const NavBuildInput& input)
{
    return input.triangles.size();
}
