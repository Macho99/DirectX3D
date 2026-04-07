#pragma once
#include "HeightFieldBase.h"

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

class CompactHeightField : public HeightFieldBase
{
public:
    CompactHeightField() = delete;
    CompactHeightField(const class HeightField& heightField, const NavBuildSettings& setting);

    const vector<CompactCell>& GetCells() const { return _cells; }
    const vector<CompactSpan>& GetSpans() const { return _spans; }

    const vector<int>& GetRegions() const { return _regions; }

private:
    void BuildRegions();
    void FloodFillRegion(int startIndex, int regionId);
    void FilterRegions(int minRegionSize);

private:
    vector<CompactCell> _cells; // width * depth
    vector<CompactSpan> _spans; // 모든 column의 span을 순서대로 저장
    vector<int> _regions;
};

