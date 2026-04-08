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

        for (int i = 0; i < (int)edges.size(); ++i)
        {
            if (visited[i])
                continue;

            auto loop = BuildOneLoop(edges, edgeMap, visited, i, maxStepCHeight);

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
                    if (s.connections[dir] != NOT_CONNECTED)
                        continue;

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
        vector<vector<Triangle>> contourTris;
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

vector<Triangle> Contours::TriangulateEarClipping(const vector<ContourVertex>& verts)
{
    vector<Triangle> tris;
    if (verts.size() < 3)
        return tris;

    vector<int> indices;
    indices.reserve(verts.size());
    for (int i = 0; i < (int)verts.size(); ++i)
        indices.push_back(i);

    int adder = 0;
    while (indices.size() > 3)
    {
        bool foundEar = false;

        adder++;
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
                    int distMinY = std::min(abs(p.y - a.y), std::min(abs(p.y - b.y), abs(p.y - c.y)));
                    if (distMinY < 10)
                    {
                        hasPointInside = true;
                        break;
                    }
                }
            }

            if (hasPointInside)
                continue;

            tris.push_back({ i0, i1, i2 });
            indices.erase(indices.begin() + i);
            foundEar = true;
            break;
        }

        if (!foundEar)
        {
            break;
        }
    }

    if (indices.size() == 3)
        tris.push_back({ indices[0], indices[1], indices[2] });

    return tris;
}