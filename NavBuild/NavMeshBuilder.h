#pragma once
#include "Types.h"
#include "HeightField.h"
#include "CompactHeightField.h"

struct NavBuildInput
{
    vector<InputTri> triangles;
    NavBuildSettings settings;
};

struct NavPath;

class NavMeshBuilder
{
public:
    bool Build(NavBuildInput input, const fs::path& savePath);
    bool TryFindPath(const Vec3& worldStart, const Vec3& worldEnd, OUT NavPath & navPath) const;
    
    bool IsBuilt() const { return _navMeshQuery != nullptr; }

    void SetDebugOnMarkWalkableTriangles(function<void(const vector<InputTri>&)> callback) { _onMarkWalkableTriangles = callback; }
    void SetDebugOnBuildHeightField(function<void(const HeightField&)> callback) { _onBuildHeightField = callback; }
    void SetDebugOnFilterHeightField(function<void(const HeightField&)> callback) { _onFilterHeightField = callback; }
    void SetDebugOnCompactHeightField(function<void(const CompactHeightField&)> callback) { _onCompactHeightField = callback; }
    void SetDebugOnBuildContours(function<void(const class Contours&)> callback) { _onBuildContours = callback; }
    void SetDebugOnSimplifyContours(function<void(const class Contours&)> callback) { _onSimplifyContours = callback; }
    void SetDebugOnBuildPolyMesh(function<void(const class PolyMeshField&)> callback) { _onBuildPolyMesh = callback; }
    void SetDebugOnBuildDetailMesh(function<void(const class DetailMeshField&)> callback) { _onBuildDetailMesh = callback; }

private:
    void MarkWalkableTriangles(NavBuildInput& input);
    //bool BuildHeightfield(const Bounds& bound, const NavBuildSettings & setting);
    //bool FilterWalkable();
    //bool BuildCompactHeightfield();
    //bool BuildRegions();
    //bool BuildContours();
    //bool BuildPolyMesh();
    //bool BuildDetailMesh();

private:
    Vec3 GetTriangleNormal(const InputTri& tri);
    
private:
    function<void(const vector<InputTri>&)> _onMarkWalkableTriangles;
    function<void(const HeightField&)> _onBuildHeightField;
    function<void(const HeightField&)> _onFilterHeightField;
    function<void(const CompactHeightField&)> _onCompactHeightField;
    function<void(const class Contours&)> _onBuildContours;
    function<void(const class Contours&)> _onSimplifyContours;
    function<void(const class PolyMeshField&)> _onBuildPolyMesh;
    function<void(const class DetailMeshField&)> _onBuildDetailMesh;

    unique_ptr<PolyMeshField> _polyMeshField;
    unique_ptr<DetailMeshField> _detailMeshField;
    unique_ptr<class NavMeshQuery> _navMeshQuery;
};

