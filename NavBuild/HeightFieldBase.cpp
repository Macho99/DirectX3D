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

void HeightFieldBase::GetWorldPos(int cx, int cz, OUT float& wx, OUT float& wz) const
{
    wx = _bmin.x + (cx + 0.5f) * _cs;
    wz = _bmin.z + (cz + 0.5f) * _cs;
}

void HeightFieldBase::GetWorldHeight(int cy, OUT float& wy) const
{
    wy = _bmin.y + cy * _ch;
}
