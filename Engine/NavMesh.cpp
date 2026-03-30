#include "pch.h"
#include "NavMesh.h"
#include "../NavBuild/NavMeshBuilder.h"

NavMesh::NavMesh() : Super(StaticType)
{
    NavMeshBuilder builder;
    NavBuildInput input;
    input.triangles = {
        {{0, 0, 0}, {1, 0, 0}, {0, 0, 1}},
        {{1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
    };
    DBG->Log(to_string(builder.Build(input)));
}

NavMesh::~NavMesh()
{
}
