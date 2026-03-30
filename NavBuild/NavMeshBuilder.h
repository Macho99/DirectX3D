#pragma once
#include "Types.h"

struct NavBuildSettings
{
    float cellSize = 0.3f;
    float cellHeight = 0.2f;

    float agentHeight = 1.8f;
    float agentRadius = 0.4f;
    float agentMaxClimb = 0.4f;
    float agentMaxSlopeDeg = 45.0f;
};

struct NavBuildInput
{
    vector<InputTri> triangles;
    NavBuildSettings settings;
};

class NavMeshBuilder
{
public:
    bool Build(const NavBuildInput& input, const fs::path& savePath);

private:
    bool MarkWalkableTriangles();
    bool BuildHeightfield();
    bool FilterWalkable();
    bool BuildCompactHeightfield();
    bool BuildRegions();
    bool BuildContours();
    bool BuildPolyMesh();
    bool BuildDetailMesh();
};

