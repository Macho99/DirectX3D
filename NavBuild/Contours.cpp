#include "pch.h"
#include "Contours.h"
#include "CompactHeightField.h"


Contours::Contours(const CompactHeightField& heightField, const NavBuildSettings& settings)
    : HeightFieldBase(heightField), _heightField(heightField)
{
    int maxStepCHeight = (int)std::ceil(settings.agentMaxClimb / _ch);
    const vector<int>& regions = heightField.GetRegions();
    for (int i = 1; i < regions.size(); ++i)
    {
        vector<ContourEdge> edges = CollectRegionEdges(heightField, i);
        vector<bool> visited(edges.size(), false);

        EdgeMap edgeMap;
        for (int edgeIdx = 0; edgeIdx < edges.size(); edgeIdx++)
        {
            edgeMap.emplace(edges[edgeIdx]._a, edgeIdx);
        }

        vector<vector<ContourVertex>> loops;

        for (int edgeIdx = 0; edgeIdx < (int)edges.size(); ++edgeIdx)
        {
            if (visited[edgeIdx])
                continue;

            auto loop = BuildOneLoop(edges, edgeMap, visited, edgeIdx, maxStepCHeight);

            if (loop.size() >= 3)
                loops.push_back(std::move(loop));
        }
        _contours.push_back(std::move(loops));
    }
}

void Contours::Simplify(float maxError)
{
    for (auto& contour : _contours)
    {
        for (int i = 0; i < contour.size(); i++)
        {
            if (contour[i].size() <= 3)
                continue;

            const vector<ContourVertex>& loop = contour[i];
            vector<ContourVertex> simplified;
            simplified.push_back(loop.front());
            simplified.push_back(loop.back());

            // 반복적으로 최대 편차 초과 점 삽입
            bool changed = true;
            while (changed)
            {
                changed = false;
                for (int i = 0; i + 1 < (int)simplified.size(); ++i)
                {
                    // raw에서 simplified[i] ~ simplified[i+1] 구간 찾기
                    int si = find(loop.begin(), loop.end(), simplified[i]) - loop.begin();
                    int ei = find(loop.begin(), loop.end(), simplified[i + 1]) - loop.begin();

                    if (ei <= si + 1) continue;

                    if (NeedsPoint(loop, si, ei, maxError))
                    {
                        // 최대 편차 점 삽입
                        float maxD = 0; int maxI = si;
                        for (int j = si + 1; j < ei; ++j)
                        {
                            float d = PerpendicularDist(loop[j], loop[si], loop[ei]);
                            if (d > maxD) { maxD = d; maxI = j; }
                        }
                        simplified.insert(simplified.begin() + i + 1, loop[maxI]);
                        changed = true;
                        break;
                    }
                }

                if (simplified.size() > loop.size())
                {
                    // maxError가 너무 작은 경우
                    simplified = loop;
                    break;
                }
            }

            struct PairHash
            {
                size_t operator()(const std::pair<int, int>& p) const noexcept
                {
                    return (static_cast<size_t>(p.first) << 32) ^ static_cast<size_t>(p.second);
                }
            };

            unordered_set<pair<int, int>, PairHash> uniqueXZ;
            uniqueXZ.reserve(simplified.size());

            // 중복 점 제거 (같은 xz 좌표에 y만 다른 경우)
            for (int j = 0; j < simplified.size(); j++)
            {
                ContourVertex& vertex = simplified[j];
                pair<int, int> xz{ vertex.x, vertex.z };
                if (uniqueXZ.count(xz) > 0)
                {
                    simplified.erase(simplified.begin() + j);
                    j--;
                }
                else
                {
                    uniqueXZ.insert(xz);
                }
            }

            // 세 점이 일직선 상에 있으면 가운데 점 제거
            for (int j = 0; j < simplified.size() - 2; j++)
            {
                ContourVertex& prev = simplified[j];
                ContourVertex& current = simplified[j + 1];
                const ContourVertex& next = simplified[j + 2];

                if (Cross2D(prev, current, next) == 0)
                {
                    if (Dot2D(prev, current, next) > 0)
                    {
                        simplified.erase(simplified.begin() + j + 1);
                        j--;
                    }
                }
            }

            contour[i] = std::move(simplified);
        }
    }
}

vector<ContourEdge> Contours::CollectRegionEdges(const CompactHeightField& heightField, uint16 targetRegion)
{
    vector<ContourEdge> edges;

    const vector<CompactCell>& cells = heightField.GetCells();
    const vector<CompactSpan>& spans = heightField.GetSpans();

    for (int cz = 0; cz < _depth; cz++)
    {
        for (int cx = 0; cx < _width; cx++)
        {
            const CompactCell& cell = cells[GetColumnIndex(cx, cz)];
            for (int i = 0; i < cell.count; ++i)
            {
                int spanIdx = cell.index + i;
                const CompactSpan& s = spans[spanIdx];
                if (s.region != targetRegion)
                    continue;

                for (int dir = 0; dir < 4; dir++)
                {
                    int nei = s.connections[dir];
                    if (nei != NOT_CONNECTED)
                    {
                        if (spans[nei].region == targetRegion)
                            continue;
                    }

                    ContourEdge edge(cx, cz, s.y, targetRegion, spanIdx, dir);
                    edges.push_back(edge);
                }
            }
        }
    }

    return edges;
}

vector<ContourVertex> Contours::BuildOneLoop(const vector<ContourEdge>& edges, const EdgeMap& edgeMap, vector<bool>& used,
    int startEdgeIdx, int maxStepCHeight)
{
    vector<ContourVertex> loop;

    const ContourEdge& start = edges[startEdgeIdx];
    used[startEdgeIdx] = true;

    loop.push_back({ start._a.x, start._y, start._a.z });
    loop.push_back({ start._b.x, start._y, start._b.z });

    ContourVertex startPoint;
    startPoint.x = start._a.x;
    startPoint.y = start._y;
    startPoint.z = start._a.z;
    ContourVertex current;
    current.x = start._b.x;
    current.y = start._y;
    current.z = start._b.z;

    while (true)
    {
        int nextIdx = -1;

        Int2 currentInt2{ current.x, current.z };
        auto range = edgeMap.equal_range(currentInt2);
        int minYDiff = INT_MAX;
        for (auto& it = range.first; it != range.second; ++it)
        {
            int idx = it->second;
            if (used[idx])
                continue;
            const ContourEdge& edge = edges[idx];
            if (edge._region != start._region)
                continue;

            int yDiff = std::abs(edge._y - current.y);
            if (yDiff < minYDiff)
            {
                minYDiff = yDiff;
                nextIdx = idx;
            }
        }

        if (nextIdx < 0)
        {
            // loop가 안 닫힘: 에러 또는 데이터 이상
            break;
        }

        used[nextIdx] = true;
        const ContourEdge& e = edges[nextIdx];

        ContourVertex next;
        next.x = e._b.x;
        next.y = e._y;
        next.z = e._b.z;

        {
            ContourVertex& prev = loop[loop.size() - 2];
            if (prev.y == current.y && current.y == next.y)
            {
                int abx = current.x - prev.x;
                int abz = current.z - prev.z;
                int bcx = next.x - current.x;
                int bcz = next.z - current.z;

                if (abx * bcz - abz * bcx == 0)
                {
                    // 일직선 상에 있으면 current는 빼도 됨
                    loop.pop_back();
                }
            }
        }

        current = next;
        loop.push_back(current);
    }
    loop.pop_back(); // 마지막은 시작과 같으므로 제거
    return loop;
}

float Contours::PerpendicularDist(ContourVertex p, ContourVertex lineStart, ContourVertex lineEnd)
{
    float dx = lineEnd.x - lineStart.x;
    float dy = lineEnd.y - lineStart.y;
    float dz = lineEnd.z - lineStart.z;

    float lenSq = dx * dx + dy * dy + dz * dz;
    if (lenSq == 0.0f)
    {
        dx = p.x - lineStart.x;
        dy = p.y - lineStart.y;
        dz = p.z - lineStart.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    float t = ((p.x - lineStart.x) * dx +
        (p.y - lineStart.y) * dy +
        (p.z - lineStart.z) * dz) / lenSq;
    t = max(0.0f, min(1.0f, t));

    float projX = lineStart.x + t * dx;
    float projY = lineStart.y + t * dy;
    float projZ = lineStart.z + t * dz;

    dx = p.x - projX;
    dy = p.y - projY;
    dz = p.z - projZ;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

bool Contours::NeedsPoint(const vector<ContourVertex>& loop, int start, int end, float maxError)
{
    float maxDist = 0.0f;

    for (int i = start + 1; i < end; ++i)
    {
        float d = PerpendicularDist(loop[i], loop[start], loop[end]);
        maxDist = max(maxDist, d);
    }

    return maxDist > maxError;
}

void Contours::BuildPolyMesh()
{
    _polyMeshs.clear();
    for (const auto& contour : _contours)
    {
        vector<PolyMesh> contourTris;
        for (const auto& loop : contour)
        {
            contourTris.push_back(TriangulateEarClipping(loop));
        }
        _polyMeshs.push_back(std::move(contourTris));
    }
}

int Contours::Cross2D(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c)
{
    int abx = b.x - a.x;
    int abz = b.z - a.z;
    int acx = c.x - a.x;
    int acz = c.z - a.z;
    return abx * acz - abz * acx;
}

int Contours::Dot2D(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c)
{
    int bax = a.x - b.x;
    int baz = a.z - b.z;
    int bcx = c.x - b.x;
    int bcz = c.z - b.z;
    return bax * bcx + baz * bcz;
}

bool Contours::IsConvex(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c)
{
    return Cross2D(a, b, c) > 0;
}

bool Contours::PointInTri2D(const ContourVertex& p, const ContourVertex& a, const ContourVertex& b, const ContourVertex& c)
{
    int c1 = Cross2D(a, b, p);
    int c2 = Cross2D(b, c, p);
    int c3 = Cross2D(c, a, p);

    bool hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0);
    bool hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0);

    return !(hasNeg && hasPos);
}

Contours::PolyMesh Contours::TriangulateEarClipping(const vector<ContourVertex>& verts)
{
    pair<vector<Triangle>, vector<ContourVertex>> result;
    if (verts.size() < 3)
        return result;

    vector<int> indices;
    indices.reserve(verts.size());
    for (int i = 0; i < (int)verts.size(); ++i)
        indices.push_back(i);

    int adder = 0;

    bool debug = true;
    while (indices.size() > 3)
    {
        bool foundEar = false;

        adder++;
        float minSampledY = FLT_MAX;
        int minSampledYIdx = -1;
        for (int idx = 0; idx < (int)indices.size(); ++idx)
        {
            //int i = (idx + adder) % indices.size();
            int i = idx;

            int i0 = indices[(i - 1 + indices.size()) % indices.size()];
            int i1 = indices[i];
            int i2 = indices[(i + 1) % indices.size()];

            const ContourVertex& a = verts[i0];
            const ContourVertex& b = verts[i1];
            const ContourVertex& c = verts[i2];

            if (!IsConvex(a, b, c))
                continue;

            bool hasPointInside = false;
            for (int j = 0; j < (int)indices.size(); ++j)
            {
                int k = indices[j];
                if (k == i0 || k == i1 || k == i2)
                    continue;

                ContourVertex p = verts[k];
                if (PointInTri2D(p, a, b, c))
                {
                    //int distMinY = std::min({ abs(p.y - a.y), abs(p.y - b.y), abs(p.y - c.y) });
                    //if (distMinY < 10)
                    //{
                    //    Triangle invalidTri{ i0, i1, i2, false };
                    //    result.first.push_back(invalidTri);
                    //    hasPointInside = true;
                    //    break;
                    //}
                    hasPointInside = true;
                    Triangle invalidTri{ i0, i1, i2, false };
                    result.first.push_back(invalidTri);
                    break;
                }
            }

            if (hasPointInside)
                continue;

            //float sampledY = SampledAverageY(a, b, c);
            //if (minSampledY >= sampledY)
            //{
            //    minSampledY = sampledY;
            //    minSampledYIdx = i;
            //}
            minSampledYIdx = i;
        }

        if (minSampledYIdx < 0) 
        {
            break;
        }

        int i = minSampledYIdx;
        int i0 = indices[(i - 1 + indices.size()) % indices.size()];
        int i1 = indices[i];
        int i2 = indices[(i + 1) % indices.size()];
        result.first.push_back({ i0, i1, i2 });
        indices.erase(indices.begin() + i);
    }

    if (indices.size() == 3)
        result.first.push_back({ indices[0], indices[1], indices[2] });
    else
    {
        for (int i = 0; i < (int)indices.size(); ++i)
        {
            result.second.push_back(verts[indices[i]]);
        }
    }

    return result;
}

float Contours::SampledAverageY(const ContourVertex& a, const ContourVertex& b, const ContourVertex& c)
{
    const vector<CompactCell>& cells = _heightField.GetCells();
    const vector<CompactSpan>& spans = _heightField.GetSpans();

    ContourVertex minBound;
    minBound.x = min({ a.x, b.x, c.x });
    minBound.y = min({ a.y, b.y, c.y });
    minBound.z = min({ a.z, b.z, c.z });

    ContourVertex maxBound;
    maxBound.x = max({ a.x, b.x, c.x });
    maxBound.y = max({ a.y, b.y, c.y });
    maxBound.z = max({ a.z, b.z, c.z });

    Vec3 v0{ (float)a.x, (float)a.y, (float)a.z };
    Vec3 v1{ (float)b.x, (float)b.y, (float)b.z };
    Vec3 v2{ (float)c.x, (float)c.y, (float)c.z };

    Vec3 e0 = v1 - v0;
    Vec3 e1 = v2 - v0;
    Vec3 n = Vec3::Cross(e0, e1);
    n.y += 0.00001f;

    float A = n.x;
    float B = n.y;
    float C = n.z;
    float D = -(A * v0.x + B * v0.y + C * v0.z);

    float yDiffSum = 0;
    int yDiffCount = 0;

    for (int cx = minBound.x; cx < maxBound.x; cx++)
    {
        for (int cz = minBound.z; cz < maxBound.z; cz++)
        {
            if (PointInTri2D({ cx, 0, cz }, a, b, c) == false)
                continue;

            float cy = -(A * cx + C * cz + D) / B;
            cy = std::clamp(cy, (float)minBound.y, (float)maxBound.y);

            float minYDiff = INT_MAX;
            int closestCellIdx = -1;
            int columnIdx = GetColumnIndex(cx, cz);
            const CompactCell cell = cells[columnIdx];
            for (int cellIdx = 0; cellIdx < cell.count; ++cellIdx)
            {
                const CompactSpan span = spans[cell.index + cellIdx];

                float yDiff = abs(span.y - cy);
                if (minYDiff > yDiff)
                {
                    minYDiff = yDiff;
                    closestCellIdx = cell.index + cellIdx;
                }
            }

            if (closestCellIdx < 0)
                continue;

            yDiffSum += minYDiff;
            yDiffCount++;
        }
    }

    return yDiffCount > 0 ? yDiffSum / yDiffCount : 0;
}
