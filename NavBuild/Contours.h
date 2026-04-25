#pragma once
#include "HeightFieldBase.h"

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

//struct ContourEdge
//{
//    ContourEdge() = delete;
//    ContourEdge(int x, int z, int y, int dir)
//        :_y(y)
//    {
//        Vertex2D p0{ x,     z };
//        Vertex2D p1{ x + 1, z };
//        Vertex2D p2{ x + 1, z + 1 };
//        Vertex2D p3{ x,     z + 1 };
//
//        switch (dir)
//        {
//        case 0: _a = p2; _b = p3; break;
//        case 1: _a = p1; _b = p2; break;
//        case 2: _a = p0; _b = p1; break;
//        case 3:
//        default:_a = p3; _b = p0; break;
//        }
//    }
//
//    Vertex2D _a;
//    Vertex2D _b;
//    int _y;
//};

struct Poly
{
    static constexpr int MAX_VERTS = 6;
    int indices[MAX_VERTS] = {};
    int vertCount = 0;
    bool isValid = true;

    Poly(const vector<int>& srcIndices, bool isValid = true)
        : isValid(isValid)
    {
        vertCount = static_cast<int>(std::min(srcIndices.size(), static_cast<size_t>(Poly::MAX_VERTS)));
        for (int i = 0; i < vertCount; ++i)
            indices[i] = srcIndices[i];
    }
};

class Contours : public HeightFieldBase
{
    using PolyMesh = pair<vector<Poly>, vector<ContourVertex>>;
public:
    Contours(const class CompactHeightField& heightField, const NavBuildSettings& settings);

    const vector<vector<ContourVertex>>& GetContours() const { return _contours; }
    //void GreedySimplify(float maxError);
    void RDPSimplify(float maxError);

private:
    using EdgeMap = unordered_multimap<Vertex2D, int, Vertex2DHash>;
    bool FindWalkStartPos(const CompactHeightField& heightField, const int region, int& startX, int& startZ, int & findSpanIdx, int & findDir);
    vector<ContourVertex> BuildOneLoopByWalking(const CompactHeightField& heightField, int startX, int startZ, int startSpan, int startDir);
    int GetNeighborSpanIdx(const CompactHeightField& heightField, int cx, int cz, int targetY);
    ContourVertex GetCornerVertex(const CompactHeightField& heightField, int cx, int cz, int spanIdx, int dir, int neighborRegion);

    void RDP(const vector<ContourVertex>& loop, vector<bool>& keep, int si, int ei, float maxError);

private:
    float PointToSegmentDist(ContourVertex p, ContourVertex lineStart, ContourVertex lineEnd);
    float PerpendicularDist(ContourVertex p, ContourVertex lineStart, ContourVertex lineEnd);

public:
    void BuildPolyMesh();
    const vector<PolyMesh>& GetPolyMeshs() const { return _polyMeshs; }

private:
    int Cross2D(const Vertex & a, const Vertex & b, const Vertex & c);
    int Dot2D(const Vertex & a, const Vertex & b, const Vertex & c);
    bool IsConvex(const Vertex & a, const Vertex & b, const Vertex & c);
    bool IsConvex(const vector<int>& poly, const vector<ContourVertex>& verts);
    bool PointInTri2D(const Vertex & p, const Vertex & a, const Vertex & b, const Vertex & c);
    PolyMesh TriangulateEarClipping(const vector<ContourVertex>& verts);

    pair<int, int> FindSharedEdge(const Poly& a, const Poly& b);
    vector<int> BuildMergedVerts(const Poly& a, int edgeA, const Poly& b, int edgeB);
    vector<Poly> MergeToConvexPolys(const vector<Poly>& triangles, const vector<ContourVertex>& positions);

private:
    vector<vector<ContourVertex>> _contours;
    vector<PolyMesh> _polyMeshs;
    const CompactHeightField& _heightField;
};