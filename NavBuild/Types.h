#pragma once

struct NavBuildSettings
{
    float cellSize = 0.3f;
    float cellHeight = 0.2f;

    float agentHeight = 1.8f;
    float agentRadius = 0.4f;
    float agentMaxClimb = 0.4f;
    float agentMaxSlopeDeg = 45.0f;

    int minRegionCount = 50;
    float contourMaxError = 1.0f;
};

struct InputTri
{
    Vec3 v0;
    Vec3 v1;
    Vec3 v2;
    bool walkable = false;
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

    bool IsInside(const Vec3& p) const
    {
        return p.x >= bmin.x && p.x <= bmax.x &&
            p.y >= bmin.y && p.y <= bmax.y &&
            p.z >= bmin.z && p.z <= bmax.z;
    }

    bool IsInside(const InputTri& tri) const
    {
        return IsInside(tri.v0) && IsInside(tri.v1) && IsInside(tri.v2);
    }
};