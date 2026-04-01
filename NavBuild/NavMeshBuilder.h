#pragma once
#include "Types.h"
#include "HeightField.h"

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
    bool Build(NavBuildInput input, const fs::path& savePath);

    void SetDebugOnMarkWalkableTriangles(function<void(const vector<InputTri>&)> callback) { _onMarkWalkableTriangles = callback; }
    void SetDebugOnBuildHeightField(function<void(const HeightField&)> callback) { _onBuildHeightField = callback; }
    void SetDebugOnFilterHeightField(function<void(const HeightField&)> callback) { _onFilterHeightField = callback; }

private:
    void MarkWalkableTriangles(NavBuildInput& input);
    //bool BuildHeightfield(const Bounds& bound, const NavBuildSettings & setting);
    //bool FilterWalkable();
    bool BuildCompactHeightfield();
    bool BuildRegions();
    bool BuildContours();
    bool BuildPolyMesh();
    bool BuildDetailMesh();

private:
    Vec3 GetTriangleNormal(const InputTri& tri);
    

private:
    function<void(const vector<InputTri>&)> _onMarkWalkableTriangles;
    function<void(const HeightField&)> _onBuildHeightField;
    function<void(const HeightField&)> _onFilterHeightField;
};

