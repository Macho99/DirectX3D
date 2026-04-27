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

    int debugSeedCount = 0;
    float detailSampleMaxError = 1.0f;
    int detailSampleDist = 2;
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

struct Vertex2D
{
    int x, z;

    bool operator==(const Vertex2D& other) const
    {
        return x == other.x && z == other.z;
    }

    bool operator!=(const Vertex2D& other) const
    {
        return !(*this == other);
    }

    int Dot(const Vertex2D& other) const
    {
        return x * other.x + z * other.z;
    }

    int LengthSq() const
    {
        return x * x + z * z;
    }
};

struct Vertex2DHash
{
    size_t operator()(const Vertex2D& p) const
    {
        return (static_cast<size_t>(p.x) * 73856093u) ^
            (static_cast<size_t>(p.z) * 19349663u);
    }
};

struct Vertex
{
    int x, y, z;
    bool operator==(const Vertex& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const Vertex& other) const
    {
        return !(*this == other);
    }
};

struct ContourVertex : public Vertex
{
    int neighborRegion;
};

struct VertexHash
{
    size_t operator()(const Vertex& v) const
    {
        return (static_cast<size_t>(v.x) * 73856093u) ^
            (static_cast<size_t>(v.y) * 19349663u) ^
            (static_cast<size_t>(v.z) * 83492791u);
    }
};

template<int MaxVerts>
struct PolyBase
{
    static constexpr int MAX_VERTS = MaxVerts;

    int indices[MAX_VERTS] = {};
    int vertCount = 0;

    PolyBase() = default;

    PolyBase(const vector<int>& srcIndices)
    {
        vertCount = static_cast<int>(std::min(srcIndices.size(), static_cast<size_t>(MAX_VERTS)));

        for (int i = 0; i < vertCount; ++i)
            indices[i] = srcIndices[i];
    }
};

struct Triangle : public PolyBase<3>
{
    Triangle() = default;
    Triangle(int i0, int i1, int i2)
        : PolyBase({ i0, i1, i2 })
    {
        indices[0] = i0;
        indices[1] = i1;
        indices[2] = i2;
        vertCount = 3;
    }
};

using Poly = PolyBase<6>;

constexpr float kEps = 1e-6f;