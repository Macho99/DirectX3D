#pragma once
#include "Types.h"
#include "HeightField.h"
#include "CompactHeightField.h"

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
    void SetDebugOnCompactHeightField(function<void(const CompactHeightField&)> callback) { _onCompactHeightField = callback; }
    void SetDebugOnBuildContours(function<void(const class Contours&)> callback) { _onBuildContours = callback; }

private:
    void MarkWalkableTriangles(NavBuildInput& input);
    //bool BuildHeightfield(const Bounds& bound, const NavBuildSettings & setting);
    //bool FilterWalkable();
    //bool BuildCompactHeightfield();
    //bool BuildRegions();
    //bool BuildContours();
    bool BuildPolyMesh();
    bool BuildDetailMesh();

private:
    Vec3 GetTriangleNormal(const InputTri& tri);
    

private:
    function<void(const vector<InputTri>&)> _onMarkWalkableTriangles;
    function<void(const HeightField&)> _onBuildHeightField;
    function<void(const HeightField&)> _onFilterHeightField;
    function<void(const CompactHeightField&)> _onCompactHeightField;
    function<void(const class Contours&)> _onBuildContours;
};

