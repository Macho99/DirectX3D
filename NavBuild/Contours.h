#pragma once
#include "HeightFieldBase.h"

struct Int2
{
    int x, z;

    bool operator==(const Int2& other) const
    {
        return x == other.x && z == other.z;
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
    ContourEdge(int x, int z, int y, uint16 region, int spanIdx, int dir)
    {
        Int2 p0{ x,     z };
        Int2 p1{ x + 1, z };
        Int2 p2{ x + 1, z + 1 };
        Int2 p3{ x,     z + 1 };

        _y = y;
        _region = region;
        _spanIndex = spanIdx;
        _dir = dir;

        switch (dir)
        {
        case 0: _a = p2; _b = p3; break;
        case 1: _a = p1; _b = p2; break;
        case 2: _a = p0; _b = p1; break;
        case 3: _a = p3; _b = p0; break;
        }
    }

    Int2 _a;
    Int2 _b;
    int _y;
    uint16 _region;
    int _spanIndex;
    int _dir;
};


class Contours : public HeightFieldBase
{
public:
    Contours(const class CompactHeightField& heightField, const NavBuildSettings& settings);

    const vector<vector<vector<ContourVertex>>>& GetContours() const { return _contours; }
    void Simplify(float maxError);

private:
    using EdgeMap = unordered_multimap<Int2, int, Int2Hash>;
    vector<ContourEdge> CollectRegionEdges(const CompactHeightField& heightField, uint16 targetRegion);
    vector<ContourVertex> BuildOneLoop(const vector<ContourEdge>& edges, const EdgeMap& edgeMap, vector<bool>& used, int startEdgeIdx, int maxStepCHeight);

private:
    float PerpendicularDist(ContourVertex p, ContourVertex lineStart, ContourVertex lineEnd);
    bool NeedsPoint(const vector<ContourVertex>& contour, int start, int end, float maxError);

private:
    vector<vector<vector<ContourVertex>>> _contours;
};

