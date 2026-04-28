#pragma once
class HeightFieldBase
{
public:
    HeightFieldBase() = delete;
    HeightFieldBase(const Bounds& bound, float cellSize, float cellHeight);
    HeightFieldBase(const HeightFieldBase& other);

public:
    int GetWidth() const { return _width; }
    int GetDepth() const { return _depth; }

    Vec3 GetBoundMin() const { return _bmin; }
    Vec3 GetBoundMax() const { return _bmax; }

    float GetCellSize() const { return _cs; }
    float GetCellHeight() const { return _ch; }

    void GetCellWorldPos(int cx, int cz, OUT float& wx, OUT float& wz) const;
    void GetVertexWorldPos(int cx, int cz, float & wx, float & wz) const;
    void GetVertexWorldPos(float cx, float cz, float& wx, float& wz) const;
    void GetWorldHeight(int cy, OUT float& wy) const;
    void GetWorldHeight(float cy, OUT float& wy) const;
    int GetColumnIndex(int cx, int cz) const { return cx + cz * _width; }

    int Cross2D(const Vertex& a, const Vertex& b, const Vertex& c) const;
    float Cross2D(const Vec3& a, const Vec3& b, const Vec3& c) const;
    int Dot2D(const Vertex& a, const Vertex& b, const Vertex& c) const;
    bool IsConvex(const Vertex& a, const Vertex& b, const Vertex& c) const;

    bool PointInTri2D(const Vertex& p, const Vertex& a, const Vertex& b, const Vertex& c) const;
    bool PointInTri2D(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) const;

    float GetTriY(float x, float z, const Vec3& v0, const Vec3& v1, const Vec3& v2) const;

protected:
    void GetCellIndex(float wx, float wz, OUT int& cx, OUT int& cz) const;
    int GetCellHeight(float wy) const;

protected:
    int _width = 0;   // x 방향 cell 개수
    int _depth = 0;   // z 방향 cell 개수

    const Vec3 _bmin;       // 월드 최소
    const Vec3 _bmax;       // 월드 최대

    const float _cs; // cell size (xz)
    const float _ch; // cell height (y)

    static const int _dx[4];
    static const int _dz[4];
};

