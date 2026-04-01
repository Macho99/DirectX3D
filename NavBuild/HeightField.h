#pragma once

struct Span
{
    uint16 cminY;     // 아래 높이 (voxel index)
    uint16 cmaxY;     // 위 높이 (voxel index)
    uint8 area;      // walkable area 등
};

class HeightField
{
public:
    HeightField() = delete;
    HeightField(const Bounds& bound, float cellSize, float cellHeight);

public:
    void HandleTriangles(const vector<InputTri>& tris);

public:
    int GetWidth() const { return _width; }
    int GetDepth() const { return _depth; }

    Vec3 GetBoundMin() const { return _bmin; }
    Vec3 GetBoundMax() const { return _bmax; }

    float GetCellSize() const { return _cs; }
    float GetCellHeight() const { return _ch; }

    const vector<vector<Span>>& GetColumns() const { return columns; }
    void GetWorldPos(int cx, int cz, OUT float& wx, OUT float& wz) const;
    void GetWorldHeight(int cy, OUT float& wy) const;
    int GetColumnIndex(int cx, int cz) const { return cx + cz * _width; }

private:
    void AddSpan(int cx, int cz, uint16 cminY, uint16 cmaxY, uint8 area);

    void GetCellIndex(float wx, float wz, OUT int & cx, OUT int & cz) const;
    int GetCellHeight(float wy) const;

private:
    int _width = 0;   // x 방향 cell 개수
    int _depth = 0;   // z 방향 cell 개수

    const Vec3 _bmin;       // 월드 최소
    const Vec3 _bmax;       // 월드 최대

    const float _cs; // cell size (xz)
    const float _ch; // cell height (y)

    vector<vector<Span>> columns; // width * depth
};
