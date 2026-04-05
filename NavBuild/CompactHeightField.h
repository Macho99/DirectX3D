#pragma once
#include "HeightFieldBase.h"
#include "HeightField.h"

struct CompactCell
{
    int index; // spans 배열 시작 위치
    int count; // 이 column에 몇 개 span 있는지
};

#define NOT_CONNECTED UINT32_MAX
struct CompactSpan
{
    uint16 y;       // 바닥 높이
    uint16 h;       // 높이 (천장까지)
    uint32 connections[4]; // 4방향 neighbor

    uint16 region = 0;
};

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

class CompactHeightField : public HeightFieldBase
{
public:
    CompactHeightField() = delete;
    CompactHeightField(const HeightField& heightField, const NavBuildSettings& setting);

    const vector<CompactCell>& GetCells() const { return _cells; }
    const vector<CompactSpan>& GetSpans() const { return _spans; }

private:
    void BuildRegions();
    void FloodFillRegion(int startIndex, int regionId);
    void FilterRegions(int minRegionSize);

public:
    vector<vector<vector<ContourVertex>>> BuildContours(float maxStepHeight);
private:
    using EdgeMap = unordered_multimap<Int2, int, Int2Hash>;
    vector<ContourEdge> CollectRegionEdges(uint16 targetRegion);
    vector<ContourVertex> BuildOneLoop(const vector<ContourEdge>& edges, const EdgeMap& edgeMap, vector<bool>& used, int startEdgeIdx, int maxStepCHeight);

private:
    vector<CompactCell> _cells; // width * depth
    vector<CompactSpan> _spans; // 모든 column의 span을 순서대로 저장
    vector<int> _regions;
};

