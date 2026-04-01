#pragma once
#include "HeightFieldBase.h"

struct Span
{
    uint16 cminY;     // 아래 높이 (voxel index)
    uint16 cmaxY;     // 위 높이 (voxel index)
    uint8 area;      // walkable area 등
};

class HeightField : public HeightFieldBase
{
public:
    HeightField() = delete;
    HeightField(const Bounds& bound, float cellSize, float cellHeight);

public:
    void HandleTriangles(const vector<InputTri>& tris);
    void FilterWalkable(float agentHeight, float agentMaxClimb);

    const vector<vector<Span>>& GetColumns() const { return columns; }

private:
    void AddSpan(int cx, int cz, uint16 cminY, uint16 cmaxY, uint8 area);

private:
    vector<vector<Span>> columns; // width * depth
};
