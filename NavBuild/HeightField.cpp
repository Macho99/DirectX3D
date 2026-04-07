#include "pch.h"
#include "HeightField.h"

HeightField::HeightField(const Bounds& bound, float cellSize, float cellHeight)
    :HeightFieldBase(bound, cellSize, cellHeight)
{
    columns.resize(_width * _depth);
}

void HeightField::HandleTriangles(const vector<InputTri>& tris)
{
    for (int triIdx = 0; triIdx < tris.size(); triIdx++)
    {
        const InputTri& tri = tris[triIdx];
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
        n.y += 0.00001f; // 수직선과의 교차점 계산 시 y값이 0이 되는 경우를 방지하기 위해 아주 작은 값을 더해줌

        float A = n.x;
        float B = n.y;
        float C = n.z;
        float D = -(A * v0.x + B * v0.y + C * v0.z);

        for (int cz = cellMinZ; cz <= cellMaxZ; cz++)
        {
            for (int cx = cellMinX; cx <= cellMaxX; cx++)
            {
                int minCellY = INT_MAX;
                int maxCellY = INT_MIN;

                int dx[] = { 0, 0, 1, 1 };
                int dz[] = { 0, 1, 0, 1 };

                bool overlaps = false;
                for (int i = 0; i < 4; i++)
                {
                    int testCx = cx + dx[i];
                    int testCz = cz + dz[i];

                    float wx = _bmin.x + testCx * _cs;
                    float wz = _bmin.z + testCz * _cs;

                    if (PointInTriangleXZ(wx, wz, tri) == false)
                        continue;

                    overlaps = true;
                    // 삼각형과 수직선(cx, 0, cz)의 교차점 계산
                    float wy = -(A * wx + C * wz + D) / B;
                    wy = std::clamp(wy, triMin.y, triMax.y);

                    int cellY = GetCellHeight(wy);
                    minCellY = std::min(minCellY, cellY);
                    maxCellY = std::max(maxCellY, cellY);
                }

                if (overlaps == false)
                {
                    if (std::abs(n.y) < 0.1f && TriangleOverlapsCell(tri, cx, cz))
                    {
                        minCellY = GetCellHeight(triMin.y);
                        maxCellY = GetCellHeight(triMax.y);
                    }
                    else
                    {
                        continue;
                    }
                }

                maxCellY = std::max(maxCellY, minCellY + 1);
                AddSpan(cx, cz, minCellY, maxCellY, tri.walkable ? 1 : 0);
            }
        }
    }
}

void HeightField::FilterWalkable(float agentHeight, float agentMaxClimb)
{
    int spanHeight = (int)std::ceil(agentHeight / _ch);
    int spanMaxClimb = (int)std::ceil(agentMaxClimb / _ch);

    // Height Filter
    for (vector<Span>& column : columns)
    {
        auto it = column.begin();
        while (it != column.end())
        {
            int ceiling = it + 1 != column.end() ? (it + 1)->cminY : INT_MAX;
            if (ceiling - it->cmaxY < spanHeight)
            {
                it->area = 0;
            }
            ++it;
        }
    }

    // Climb Filter
    // 해당 필터링 되는 조건이 드물고, 괜히 성능만 저하시키는 경우가 많아서 일단 스킵
    /*for (int columnIdx = 0; columnIdx < columns.size(); columnIdx++)
    {
        int cx = columnIdx % _width;
        int cz = columnIdx / _width;
        vector<Span>& column = columns[columnIdx];
        int dx[] = {-1, 0, 1, 0};
        int dz[] = { 0, 1, 0, -1 };

        for (Span& span : column)
        {
            if (span.area == 0)
                continue;

            bool hasWalkableNeighbor = false;
            for (int d = 0; d < 4; d++)
            {
                int ncx = cx + dx[d];
                int ncz = cz + dz[d];
                if (ncx < 0 || ncx >= _width || ncz < 0 || ncz >= _depth)
                    continue;

                vector<Span>& neighborColumn = columns[GetColumnIndex(ncx, ncz)];
                for (const Span& neighborSpan : column)
                {
                    if (neighborSpan.area == 0)
                        continue;

                    if (std::abs(span.cmaxY - neighborSpan.cmaxY) <= spanMaxClimb)
                    {
                        hasWalkableNeighbor = true;
                        break;
                    }
                }
                if (hasWalkableNeighbor)
                    break;
            }

            if (hasWalkableNeighbor == false)
            {
                span.area = 0;
            }
        }
    }*/

    // Ledge Filter
    // Skip
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

bool HeightField::TriangleOverlapsCell(const InputTri& tri, int cx, int cz)
{
    const float cellMinX = _bmin.x + cx * _cs;
    const float cellMaxX = _bmin.x + (cx + 1) * _cs;
    const float cellMinZ = _bmin.z + cz * _cs;
    const float cellMaxZ = _bmin.z + (cz + 1) * _cs;

    const Vec3 v0 = tri.v0;
    const Vec3 v1 = tri.v1;
    const Vec3 v2 = tri.v2;

    // 셀 중심과 반너비
    float centerX = (cellMinX + cellMaxX) * 0.5f;
    float centerZ = (cellMinZ + cellMaxZ) * 0.5f;
    float hx = (cellMaxX - cellMinX) * 0.5f;
    float hz = (cellMaxZ - cellMinZ) * 0.5f;

    // 삼각형 꼭짓점을 셀 중심 기준으로 이동
    float t0x = v0.x - centerX, t0z = v0.z - centerZ;
    float t1x = v1.x - centerX, t1z = v1.z - centerZ;
    float t2x = v2.x - centerX, t2z = v2.z - centerZ;

    // ── 검사 1: AABB 축 (X, Z) ──────────────────────────────
    // X축
    if (min({ t0x, t1x, t2x }) > hx) return false;
    if (max({ t0x, t1x, t2x }) < -hx) return false;
    // Z축
    if (min({ t0z, t1z, t2z }) > hz) return false;
    if (max({ t0z, t1z, t2z }) < -hz) return false;

    // ── 검사 2: 삼각형 엣지 법선 축 (3개) ──────────────────
    // 각 엣지에 수직인 축으로 투영해서 분리 가능한지 확인
    auto testEdge = [&](float ex, float ez) -> bool
        {
            // 삼각형 투영
            float p0 = t0x * ex + t0z * ez;
            float p1 = t1x * ex + t1z * ez;
            float p2 = t2x * ex + t2z * ez;
            float triMin = min({ p0, p1, p2 });
            float triMax = max({ p0, p1, p2 });

            // AABB 투영 (중심이 원점이므로 반너비만으로 계산)
            float boxR = hx * fabsf(ex) + hz * fabsf(ez);

            return triMax < -boxR || triMin > boxR; // true면 분리됨 → 겹치지 않음
        };

    // 엣지 0: v0 → v1
    float e0x = t1x - t0x, e0z = t1z - t0z;
    if (testEdge(-e0z, e0x)) return false;

    // 엣지 1: v1 → v2
    float e1x = t2x - t1x, e1z = t2z - t1z;
    if (testEdge(-e1z, e1x)) return false;

    // 엣지 2: v2 → v0
    float e2x = t0x - t2x, e2z = t0z - t2z;
    if (testEdge(-e2z, e2x)) return false;

    return true; // 분리축 없음 → 겹침
}

bool HeightField::PointInTriangleXZ(float wx, float wz, const InputTri& tri)
{
    const Vec3 v0 = tri.v0;
    const Vec3 v1 = tri.v1;
    const Vec3 v2 = tri.v2;

    // 각 엣지에 대해 외적의 Z 성분 부호 확인
    auto cross2D = [](float ax, float az, float bx, float bz)
        {
            return ax * bz - az * bx;
        };

    float d0 = cross2D(v1.x - v0.x, v1.z - v0.z, wx - v0.x, wz - v0.z);
    float d1 = cross2D(v2.x - v1.x, v2.z - v1.z, wx - v1.x, wz - v1.z);
    float d2 = cross2D(v0.x - v2.x, v0.z - v2.z, wx - v2.x, wz - v2.z);

    bool hasNeg = (d0 < 0) || (d1 < 0) || (d2 < 0);
    bool hasPos = (d0 > 0) || (d1 > 0) || (d2 > 0);

    // 부호가 섞이면 외부, 전부 같으면 내부
    return !(hasNeg && hasPos);
}
