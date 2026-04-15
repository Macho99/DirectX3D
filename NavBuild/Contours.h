#pragma once
#include "HeightFieldBase.h"

struct Int2
{
    int x, z;

    bool operator==(const Int2& other) const
    {
        return x == other.x && z == other.z;
    }

    bool operator!=(const Int2& other) const
    {
        return !(*this == other);
    }
};

struct Int2Hash
{
    size_t operator()(const Int2& p) const
    {
        return (static_cast<size_t>(p.x) * 73856093u) ^
            (static_cast<size_t>(p.z) * 19349663u);
    }
};

struct ContourVertex
{
    int x, y, z;

    bool operator==(const ContourVertex& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct ContourEdge
{
    ContourEdge() = delete;
    ContourEdge(int x, int z, int y, int dir)
        :_y(y)
    {
        Int2 p0{ x,     z };
        Int2 p1{ x + 1, z };
        Int2 p2{ x + 1, z + 1 };
        Int2 p3{ x,     z + 1 };

        switch (dir)
        {
        case 0: _a = p2; _b = p3; break;
        case 1: _a = p1; _b = p2; break;
        case 2: _a = p0; _b = p1; break;
        case 3:
        default:_a = p3; _b = p0; break;
        }
    }

    Int2 _a;
    Int2 _b;
    int _y;
};

struct ContourShareEdge : public ContourEdge
{
    ContourShareEdge() = delete;
    ContourShareEdge(int x, int z, int y, int dir, int rhsRegion, int lhsRegion)
        : ContourEdge(x, z, y, dir), _rhsRegion(rhsRegion), _lhsRegion(lhsRegion)
    {
    }

    int _rhsRegion;
    // 왼쪽 region일 경우, 방향을 뒤집어서 봐야함
    int _lhsRegion;
};

struct Triangle
{
    int i0, i1, i2;
    bool isValid = true;
};

class Contours : public HeightFieldBase
{
    using PolyMesh = pair<vector<Triangle>, vector<ContourVertex>>;
public:
    Contours(const class CompactHeightField& heightField, const NavBuildSettings& settings);

    const vector<vector<ContourVertex>>& GetContours() const { return _contours; }
    void Simplify(float maxError);
    void GetVertexWorldPos(int x, int z, float& worldX, float& worldZ) const;

private:
    using EdgeMap = unordered_multimap<Int2, int, Int2Hash>;
    bool FindWalkStartPos(const CompactHeightField& heightField, const int region, int& startX, int& startZ, int & findSpanIdx, int & findDir);
    vector<ContourVertex> BuildOneLoopByWalking(const CompactHeightField& heightField, int startX, int startZ, int startSpan, int startDir);
    int GetNeighborRegion(const CompactHeightField& heightField, int cx, int cz, int spanIdx, int dir);
    ContourVertex GetCornerVertex(const CompactHeightField& heightField, int cx, int cz, int spanIdx, int dir);

    void CollectRegionEdges(const CompactHeightField& heightField, vector<ContourShareEdge>& sharedEdges, vector<vector<ContourEdge>>& regionEdges);
    vector<ContourVertex> BuildOneLoop(const vector<ContourEdge>& edges, const EdgeMap& edgeMap, vector<bool>& used, int startEdgeIdx);
    vector<ContourVertex> BuildOneLoop(const vector<ContourShareEdge>& sharedEdges, const EdgeMap& sharedEdgeMap, vector<bool>& sharedVisited, 
        const vector<ContourEdge>& regionEdges, const EdgeMap& regionEdgeMap, vector<bool>& regionVisited, int curRegion, Int2 startPos);

private:
    float PerpendicularDist(ContourVertex p, ContourVertex lineStart, ContourVertex lineEnd);
    bool NeedsPoint(const vector<ContourVertex>& contour, int start, int end, float maxError);

public:
    void BuildPolyMesh();
    const vector<PolyMesh>& GetPolyMeshs() const { return _polyMeshs; }
private:
    int Cross2D(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c);
    int Dot2D(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c);
    bool IsConvex(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c);
    bool PointInTri2D(const ContourVertex& p, const ContourVertex& a, const ContourVertex& b, const ContourVertex& c);
    PolyMesh TriangulateEarClipping(const vector<ContourVertex>& verts);

    float SampledAverageY(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c);

private:
    vector<vector<ContourVertex>> _contours;
    vector<PolyMesh> _polyMeshs;
    const CompactHeightField& _heightField;
};