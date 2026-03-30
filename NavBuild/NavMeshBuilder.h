#pragma once

struct InputTri
{
    Vec3 v0;
    Vec3 v1;
    Vec3 v2;
};

struct Bounds
{
    Vec3 bmin = { FLT_MAX, FLT_MAX, FLT_MAX };
    Vec3 bmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    void Encapsulate(const Vec3& p)
    {
        bmin.x = std::min(bmin.x, p.x);
        bmin.y = std::min(bmin.y, p.y);
        bmin.z = std::min(bmin.z, p.z);

        bmax.x = std::max(bmax.x, p.x);
        bmax.y = std::max(bmax.y, p.y);
        bmax.z = std::max(bmax.z, p.z);
    }
};

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
    std::vector<InputTri> triangles;
    Bounds bounds;
    NavBuildSettings settings;
};

class NavMeshBuilder
{
public:
    int Build(const NavBuildInput& input);

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

