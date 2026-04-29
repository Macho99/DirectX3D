#include "pch.h"
#include "HeightFieldBase.h"
                                    //го, ©Л, ╩С, аб
const int HeightFieldBase::_dx[4] = { 0 , 1, 0, -1 };
const int HeightFieldBase::_dz[4] = { 1, 0, -1, 0 };

HeightFieldBase::HeightFieldBase(const Bounds& bound, float cellSize, float cellHeight)
    :_bmin(bound.bmin), _bmax(bound.bmax), _cs(cellSize), _ch(cellHeight)
{
    _width = (int)std::floor((_bmax.x - _bmin.x) / _cs + 0.5f);
    _depth = (int)std::floor((_bmax.z - _bmin.z) / _cs + 0.5f);
}

HeightFieldBase::HeightFieldBase(const HeightFieldBase& other)
    :_bmin(other._bmin), _bmax(other._bmax), _cs(other._cs), _ch(other._ch), _width(other._width), _depth(other._depth)
{
    _width = (int)std::floor((_bmax.x - _bmin.x) / _cs + 0.5f);
    _depth = (int)std::floor((_bmax.z - _bmin.z) / _cs + 0.5f);
}


void HeightFieldBase::GetCellIndex(float wx, float wz, OUT int& cx, OUT int& cz) const
{
    cx = (int)((wx - _bmin.x) / _cs);
    cz = (int)((wz - _bmin.z) / _cs);
}

int HeightFieldBase::GetCellHeight(float wy) const
{
    return (int)((wy - _bmin.y) / _ch);
}

void HeightFieldBase::GetCellWorldPos(int cx, int cz, OUT float& wx, OUT float& wz) const
{
    wx = _bmin.x + (cx + 0.5f) * _cs;
    wz = _bmin.z + (cz + 0.5f) * _cs;
}

void HeightFieldBase::GetVertexWorldPos(int cx, int cz, float & wx, float & wz) const
{
    wx = _bmin.x + cx * _cs;
    wz = _bmin.z + cz * _cs;
}

void HeightFieldBase::GetVertexWorldPos(float cx, float cz, float& wx, float& wz) const
{
    wx = _bmin.x + cx * _cs;
    wz = _bmin.z + cz * _cs;
}

void HeightFieldBase::GetWorldHeight(int cy, OUT float& wy) const
{
    wy = _bmin.y + cy * _ch;
}

void HeightFieldBase::GetWorldHeight(float cy, OUT float& wy) const
{
    wy = _bmin.y + cy * _ch;
}

Vec3 HeightFieldBase::ToWorldPos(const Vec3& v) const
{
    return Vec3(_bmin.x + v.x * _cs, _bmin.y + v.y * _ch, _bmin.z + v.z * _cs);
}

Vec3 HeightFieldBase::ToNavPos(const Vec3& v) const
{
    return Vec3((v.x - _bmin.x) / _cs, (v.y - _bmin.y) / _ch, (v.z - _bmin.z) / _cs);  
}


int HeightFieldBase::Cross2D(const Vertex& a, const Vertex& b, const Vertex& c) const
{
    int abx = b.x - a.x;
    int abz = b.z - a.z;
    int acx = c.x - a.x;
    int acz = c.z - a.z;
    return abx * acz - abz * acx;
}

float HeightFieldBase::Cross2D(const Vec3& a, const Vec3& b, const Vec3& c) const
{
    float abx = b.x - a.x;
    float abz = b.z - a.z;
    float acx = c.x - a.x;
    float acz = c.z - a.z;
    return abx * acz - abz * acx;
}

int HeightFieldBase::Dot2D(const Vertex& a, const Vertex& b, const Vertex& c) const
{
    int bax = a.x - b.x;
    int baz = a.z - b.z;
    int bcx = c.x - b.x;
    int bcz = c.z - b.z;
    return bax * bcx + baz * bcz;
}

bool HeightFieldBase::IsConvex(const Vertex& a, const Vertex& b, const Vertex& c) const
{
    return Cross2D(a, b, c) > 0;
}

bool HeightFieldBase::PointInTri2D(const Vertex& p, const Vertex& a, const Vertex& b, const Vertex& c) const
{
    int c1 = Cross2D(a, b, p);
    int c2 = Cross2D(b, c, p);
    int c3 = Cross2D(c, a, p);

    bool hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0);
    bool hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0);

    return !(hasNeg && hasPos);
}

bool HeightFieldBase::PointInTri2D(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) const
{
    float c1 = Cross2D(a, b, p);
    float c2 = Cross2D(b, c, p);
    float c3 = Cross2D(c, a, p);

    bool hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0);
    bool hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0);

    return !(hasNeg && hasPos);
}

float HeightFieldBase::GetTriY(float x, float z, const Vec3& v0, const Vec3& v1, const Vec3& v2) const
{
    Vec3 e0 = v1 - v0;
    Vec3 e1 = v2 - v0;
    Vec3 n = e0.Cross(e1);

    float A = n.x;
    float B = n.y;
    float C = n.z;
    float D = -(A * v0.x + B * v0.y + C * v0.z);

    float y = -(A * x + C * z + D) / B;
    return y;
}
