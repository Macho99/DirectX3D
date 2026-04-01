#include "pch.h"
#include "HeightField.h"

HeightField::HeightField(const Bounds& bound, float cellSize, float cellHeight)
    :_bmin(bound.bmin), _bmax(bound.bmax), _cs(cellSize), _ch(cellHeight)
{
    _width = (int)std::floor((_bmax.x - _bmin.x) / _cs + 0.5f);
    _depth = (int)std::floor((_bmax.z - _bmin.z) / _cs + 0.5f);

    columns.resize(_width * _depth);
}

void HeightField::HandleTriangles(const vector<InputTri>& tris)
{
    for (const InputTri& tri : tris)
    {
        const Vec3 v0 = tri.v0;
        const Vec3 v1 = tri.v1;
        const Vec3 v2 = tri.v2;

        Vec3 triMin, triMax;
        triMin.x = min(v0.x, min(v1.x, v2.x));
        triMin.y = min(v0.y, min(v1.y, v2.y));
        triMin.z = min(v0.z, min(v1.z, v2.z));

        triMax.x = max(v0.x, max(v1.x, v2.x));
        triMax.y = max(v0.y, max(v1.y, v2.y));
        triMax.z = max(v0.z, max(v1.z, v2.z));

        int cellMinX = (int)((triMin.x - _bmin.x) / _cs);
        int cellMaxX = (int)((triMax.x - _bmin.x) / _cs);
        int cellMinZ = (int)((triMin.z - _bmin.z) / _cs);
        int cellMaxZ = (int)((triMax.z - _bmin.z) / _cs); 
        cellMinX = std::max(0, std::min(cellMinX, _width - 1));
        cellMaxX = std::max(0, std::min(cellMaxX, _width - 1));
        cellMinZ = std::max(0, std::min(cellMinZ, _depth - 1));
        cellMaxZ = std::max(0, std::min(cellMaxZ, _depth - 1));

        Vec3 e0 = v1 - v0;
        Vec3 e1 = v2 - v0;
        Vec3 n = Vec3::Cross(e0, e1);

        float A = n.x;
        float B = n.y;
        float C = n.z;
        float D = -(A * v0.x + B * v0.y + C * v0.z);

        for (int cz = cellMinZ; cz <= cellMaxZ; cz++)
        {
            for (int cx = cellMinX; cx <= cellMaxX; cx++)
            {
                float wx = _bmin.x + cx * _cs + _cs * 0.5f;
                float wz = _bmin.z + cz * _cs + _cs * 0.5f;
                // 삼각형과 수직선(cx, 0, cz)의 교차점 계산
                float wy = -(A * wx + C * wz + D) / B;
                wy = std::clamp(wy, triMin.y, triMax.y);
                int cellY = GetCellHeight(wy);

                AddSpan(cx, cz, cellY, cellY + 1, tri.walkable ? 1 : 0);
            }
        }
    }
}

void HeightField::AddSpan(int cx, int cz, uint16 cminY, uint16 cmaxY, uint8 area)
{
    int columnIndex = GetColumnIndex(cx, cz);
    vector<Span>& column = columns[columnIndex];

    Span newSpan{ cminY, cmaxY, area };

    auto it = column.begin();

    // 1. 새 span보다 완전히 아래에 있는 span들은 스킵
    while (it != column.end() && it->cmaxY < newSpan.cminY)
    {
        ++it;
    }

    // 2. 겹치거나 맞닿는 span들을 전부 병합
    while (it != column.end() && it->cminY <= newSpan.cmaxY)
    {
        if(newSpan.cmaxY == it->cmaxY)
            newSpan.area = std::max(newSpan.area, it->area);
        else
            newSpan.area = newSpan.cmaxY > it->cmaxY ? newSpan.area : it->area;

        newSpan.cminY = std::min(newSpan.cminY, it->cminY);
        newSpan.cmaxY = std::max(newSpan.cmaxY, it->cmaxY);

        it = column.erase(it);
    }

    // 3. 병합 완료된 span 삽입
    column.insert(it, newSpan);
}

void HeightField::GetCellIndex(float wx, float wz, OUT int& cx, OUT int& cz) const
{
    cx = (int)((wx - _bmin.x) / _cs);
    cz = (int)((wz - _bmin.z) / _cs);
}

int HeightField::GetCellHeight(float wy) const
{
    return (int)((wy - _bmin.y) / _ch);
}

void HeightField::GetWorldPos(int cx, int cz, OUT float& wx, OUT float& wz) const
{
    wx = _bmin.x + (cx + 0.5f) * _cs;
    wz = _bmin.z + (cz + 0.5f) * _cs;
}

void HeightField::GetWorldHeight(int cy, OUT float& wy) const
{
    wy = _bmin.y + cy * _ch;
}
