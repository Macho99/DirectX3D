#include "pch.h"
#include "Contours.h"
#include "CompactHeightField.h"

Contours::Contours(const CompactHeightField& heightField, const NavBuildSettings& settings)
    : HeightFieldBase(heightField), _heightField(heightField)
{
    int maxStepCHeight = (int)std::ceil(settings.agentMaxClimb / _ch);
    const vector<int>& regions = heightField.GetRegions();

    _contours.resize(regions.size());
    for (int regionIdx = 1; regionIdx < regions.size(); ++regionIdx)
    {
        int startX, startZ, spanIdx, dir;
        bool findResult = FindWalkStartPos(heightField, regionIdx, OUT startX, OUT startZ, OUT spanIdx, OUT dir);

        if (findResult == false)
        {
            continue;
        }

        vector<ContourVertex> loop = BuildOneLoopByWalking(heightField, startX, startZ, spanIdx, dir);
        _contours[regionIdx] = std::move(loop);
    }
}

/*
void Contours::Simplify(float maxError)
{
    for (int regionIdx = 0; regionIdx < _contours.size(); regionIdx++)
    {
        const vector<ContourVertex>& originLoop = _contours[regionIdx];
        if (originLoop.size() <= 3)
            continue;

        vector<ContourVertex> loop;
        vector<ContourVertex> simplified;

        for (int i = 0; i < originLoop.size(); i++)
        {
            int prevIdx = (i - 1 + originLoop.size()) % originLoop.size();
            int prevRegion = originLoop[prevIdx].neighborRegion;

            const ContourVertex& vertex = originLoop[i];
            const int neiborRegion = vertex.neighborRegion;
            if (neiborRegion != prevRegion)
            {
                loop.push_back(vertex);
                simplified.push_back(vertex);
                prevRegion = neiborRegion;
            }
            else if (neiborRegion == -1)
            {
                loop.push_back(vertex);
            }
            else
            {
                // neiborRegion이 존재하면서, 같은 경우는 둘다 넣지 않음
            }
        }
        if(simplified.empty())
            simplified.push_back(originLoop.front());
        //simplified.push_back(originLoop.back());

        // 반복적으로 최대 편차 초과 점 삽입
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (int i = 0; i < simplified.size(); ++i)
            {
                // raw에서 simplified[i] ~ simplified[i+1] 구간 찾기
                int si = find(loop.begin(), loop.end(), simplified[i]) - loop.begin();
                int ei;
                if (i + 1 == simplified.size())
                    ei = (si - 1 + loop.size()) % loop.size();
                else
                    ei = find(loop.begin(), loop.end(), simplified[i + 1]) - loop.begin();

                // 최대 편차 점 삽입
                float maxD = 0;
                int maxI = si;
                int j = (si + 1) % loop.size();
                while (j != ei)
                {
                    float d = PerpendicularDist(loop[j], loop[si], loop[ei]);
                    if (d > maxD) 
                    { 
                        maxD = d; 
                        maxI = j; 
                    }
                    j = (j + 1) % loop.size();
                }

                if (maxD > maxError)
                {
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
        _contours[regionIdx] = std::move(simplified);
    }

        //struct PairHash
        //{
        //    size_t operator()(const std::pair<int, int>& p) const noexcept
        //    {
        //        return (static_cast<size_t>(p.first) << 32) ^ static_cast<size_t>(p.second);
        //    }
        //};
        //
        //unordered_set<pair<int, int>, PairHash> uniqueXZ;
        //uniqueXZ.reserve(simplified.size());
        //
        //// 중복 점 제거 (같은 xz 좌표에 y만 다른 경우)
        //for (int j = 0; j < simplified.size(); j++)
        //{
        //    ContourVertex& vertex = simplified[j];
        //    pair<int, int> xz{ vertex.x, vertex.z };
        //    if (uniqueXZ.count(xz) > 0)
        //    {
        //        simplified.erase(simplified.begin() + j);
        //        j--;
        //    }
        //    else
        //    {
        //        uniqueXZ.insert(xz);
        //    }
        //}
        //
        //// 세 점이 일직선 상에 있으면 가운데 점 제거
        //for (int j = 0; j < simplified.size() - 2; j++)
        //{
        //    ContourVertex& prev = simplified[j];
        //    ContourVertex& current = simplified[j + 1];
        //    const ContourVertex& next = simplified[j + 2];
        //
        //    if (Cross2D(prev, current, next) == 0)
        //    {
        //        if (Dot2D(prev, current, next) > 0)
        //        {
        //            simplified.erase(simplified.begin() + j + 1);
        //            j--;
        //        }
        //    }
        //}
}*/

void Contours::Simplify(float maxError)
{
    for (int regionIdx = 0; regionIdx < _contours.size(); regionIdx++)
    {
        const vector<ContourVertex>& loop = _contours[regionIdx];

        vector<pair<ContourVertex, int>> simplified;
        simplified.reserve(loop.size());
        for (int i = 0; i < loop.size(); i++)
        {
            const ContourVertex& vertex = loop[i];
            simplified.push_back({ vertex, i });
        }

        for (int i = 0; i < simplified.size(); )
        {
            const int n = simplified.size();
            int prev = (i - 1 + n) % n;
            int next = (i + 1) % n;

            const ContourVertex& vPrev = loop[simplified[prev].second];
            const ContourVertex& vCurr = loop[simplified[i].second];
            const ContourVertex& vNext = loop[simplified[next].second];

            // 이웃 region이 바뀌는 지점 = 두 region의 공유 경계 끝점
            if (vPrev.neighborRegion != vCurr.neighborRegion)
            {
                ++i;
                continue;  // 유지
            }
            // 바뀌지 않는 공유 경계는 제거
            else if (vCurr.neighborRegion != -1)
            {
                simplified.erase(i + simplified.begin());
                continue;
            }

            float dist = PointToSegmentDist(vCurr, vPrev, vNext);
            if (dist > maxError)
            {
                ++i;
                continue;  // 오차 커서 유지
            }
            // 제거 가능
            simplified.erase(i + simplified.begin());
            // n 갱신 후 i는 그대로 (다음 정점이 당겨옴)
        }

        _contours[regionIdx].clear();
        for (const auto& pair : simplified)
        {
            _contours[regionIdx].push_back(pair.first);
        }
    }
}

void Contours::GetVertexWorldPos(int x, int z, float& worldX, float& worldZ) const
{
    worldX = _bmin.x + x * _cs;
    worldZ = _bmin.z + z * _cs;
}

void Contours::CollectRegionEdges(const CompactHeightField& heightField, vector<ContourShareEdge>& sharedEdges, vector<vector<ContourEdge>>& regionEdges)
{
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

                for (int dir = 0; dir < 4; dir++)
                {
                    int nei = s.connections[dir];
                    if (nei != NOT_CONNECTED)
                    {
                        const CompactSpan& neighborSpan = spans[nei];
                        if (neighborSpan.region == s.region)
                            continue;

                        uint16 edgeY = (s.y + neighborSpan.y) / 2;
                        //하, 우, 상, 좌
                        //공유 엣지일 경우 각 셀의 우측과 하단만 처리(중복 방지)
                        if (dir == 0 || dir == 1)
                        {
                            ContourShareEdge sharedEdge(cx, cz, edgeY, dir, s.region, neighborSpan.region);
                            sharedEdges.push_back(sharedEdge);
                        }
                        continue;
                    }

                    ContourEdge edge(cx, cz, s.y, dir);
                    regionEdges[s.region].push_back(edge);
                }
            }
        }
    }
}

bool Contours::FindWalkStartPos(const CompactHeightField& heightField, const int region, int& startX, int& startZ, int& findSpanIdx, int& findDir)
{
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
                if (s.region != region)
                    continue;

                for (int dir = 0; dir < 4; dir++)
                {
                    int nei = s.connections[dir];
                    if (nei != NOT_CONNECTED)
                    {
                        const CompactSpan& neighborSpan = spans[nei];
                        if (neighborSpan.region == s.region)
                            continue;
                    }

                    startX = cx;
                    startZ = cz;
                    findDir = dir;
                    findSpanIdx = spanIdx;
                    return true;
                }
            }
        }
    }
    return false;
}

vector<ContourVertex> Contours::BuildOneLoopByWalking(const CompactHeightField& heightField, int startX, int startZ, int startSpan, int startDir)
{
    vector<ContourVertex> loop;
    const vector<CompactSpan>& spans = heightField.GetSpans();

    //unordered_set<Int2, Int2Hash> visited;

    //하, 우, 상, 좌
    int x = startX;
    int z = startZ;
    int spanIdx = startSpan;
    int dir = startDir;
    do
    {
        const CompactSpan& span = spans[spanIdx];
        int nx = x + _dx[dir];
        int nz = z + _dz[dir];

        int neiIdx = GetNeighborSpanIdx(heightField, nx, nz, span.y);
        int neighborRegion = (neiIdx != -1) ? spans[neiIdx].region : -1;
        // 경계일 경우 기록 후 우회전
        if (neighborRegion != span.region)
        {
            ContourVertex vertex = GetCornerVertex(heightField, x, z, spanIdx, dir, neighborRegion);

            //if (visited.count(Int2{ vertex.x, vertex.z }) > 0)
            //{
            //    while (loop.back().x != vertex.x || loop.back().z != vertex.z)
            //    {
            //        loop.pop_back();
            //    }
            //}
            //else
            {
                //visited.insert(Int2{ vertex.x, vertex.z });
                loop.push_back(vertex);
            }

            dir = (dir + 3) % 4;
        }
        // 접근 가능할 경우 직진 및 좌회전
        else
        {
            spanIdx = neiIdx;
            x = nx;
            z = nz;
            dir = (dir + 1) % 4;
        }

    } while (x != startX || z != startZ || dir != startDir);

    return loop;
}

int Contours::GetNeighborSpanIdx(const CompactHeightField& heightField, int cx, int cz, int targetY)
{
    const vector<CompactSpan>& spans = heightField.GetSpans();
    const vector<CompactCell>& cells = heightField.GetCells();
    const int maxClimbCell = heightField.GetAgentMaxClimbCell();

    if (cx < 0 || cx >= _width || cz < 0 || cz >= _depth)
        return -1;

    const CompactCell& neighborCell = cells[heightField.GetColumnIndex(cx, cz)];
    for (int i = 0; i < neighborCell.count; i++)
    {
        int neighborSpanIdx = neighborCell.index + i;
        const CompactSpan& neighborSpan = spans[neighborSpanIdx];
        if (neighborSpan.region != 0 && std::abs(targetY - neighborSpan.y) <= maxClimbCell * 2)
        {
            return neighborSpanIdx;
        }
    }
    return -1;
}

ContourVertex Contours::GetCornerVertex(const CompactHeightField& heightField, int cx, int cz, int spanIdx, int dir, int neighborRegion)
{
    Int2 vertexInt2;
    int dirFirst = dir;
    int dirSecond = (dir + 1) % 4;

    switch (dir)
    {
    case 0:
        vertexInt2 = { cx + 1, cz + 1 };
        break;
    case 1:
        vertexInt2 = { cx + 1, cz };
        break;
    case 2: 
        vertexInt2 = { cx,     cz };
        break;
    case 3:
    default:
        vertexInt2 = { cx,     cz + 1 };
        break;
    }
    const vector<CompactSpan>& spans = heightField.GetSpans();
    const int targetY = spans[spanIdx].y;

    int ySum = targetY;
    int yCount = 1;

    auto CheckNeighbor = [&](int neighborSpanIdx)
        {
            if (neighborSpanIdx != NOT_CONNECTED)
            {
                const CompactSpan& neighborSpan = spans[neighborSpanIdx];
                ySum += neighborSpan.y;
                yCount++;
            }
        };

    CheckNeighbor(GetNeighborSpanIdx(heightField, cx + _dx[dirFirst], cz + _dz[dirFirst], targetY));
    CheckNeighbor(GetNeighborSpanIdx(heightField, cx + _dx[dirSecond], cz + _dz[dirSecond], targetY));
    CheckNeighbor(GetNeighborSpanIdx(heightField, cx + _dx[dirFirst] + _dx[dirSecond], cz + _dz[dirFirst] + _dz[dirSecond], targetY));

    ContourVertex vertex;
    vertex.x = vertexInt2.x;
    vertex.y = ySum / yCount;
    vertex.z = vertexInt2.z;
    vertex.neighborRegion = neighborRegion;
    return vertex;
}

vector<ContourVertex> Contours::BuildOneLoop(const vector<ContourEdge>& edges, const EdgeMap& edgeMap, vector<bool>& used,
    int startEdgeIdx)
{
    vector<ContourVertex> loop;

    const ContourEdge& start = edges[startEdgeIdx];
    used[startEdgeIdx] = true;

    ContourVertex current;
    current.x = start._a.x;
    current.y = start._y;
    current.z = start._a.z;

    loop.push_back(current);

    int currentY = start._y;
    Int2 nextInt2{ start._b.x, start._b.z };

    while (true)
    {
        int nextIdx = -1;

        auto range = edgeMap.equal_range(nextInt2);
        int minYDiff = INT_MAX;
        for (auto& it = range.first; it != range.second; ++it)
        {
            int idx = it->second;
            if (used[idx])
                continue;
            const ContourEdge& edge = edges[idx];

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

        if (current.x < e._a.x || current.z < e._a.z)
        {
            current.y = e._y;
        }
        else
        {
            current.y = currentY;
        }

        //current.y = (currentY + e._y) / 2;
        current.x = e._a.x;
        current.z = e._a.z;

        currentY = e._y;
        loop.push_back(current);

        nextInt2 = Int2{ e._b.x, e._b.z };
    }
    return loop;
}

vector<ContourVertex> Contours::BuildOneLoop(const vector<ContourShareEdge>& sharedEdges, const EdgeMap& sharedEdgeMap, vector<bool>& sharedVisited, 
    const vector<ContourEdge>& regionEdges, const EdgeMap& regionEdgeMap, vector<bool>& regionVisited, int curRegion, Int2 startPos)
{
    vector<ContourVertex> loop;

    Int2 currentInt2 = startPos;
    while (true)
    {
        bool findEdge = false;
        int currentY = 0;
        Int2 nextInt2;
        {
            auto range = sharedEdgeMap.equal_range(currentInt2);
            for (auto& it = range.first; it != range.second; ++it)
            {
                int idx = it->second;
                if (sharedVisited[idx])
                    continue;

                const ContourShareEdge& shareEdge = sharedEdges[idx];
                Int2 edgeStart;
                Int2 edgeEnd;
                if (shareEdge._rhsRegion == curRegion)
                {
                    edgeStart = shareEdge._a;
                    edgeEnd = shareEdge._b;
                }
                else
                {
                    edgeStart = shareEdge._b;
                    edgeEnd = shareEdge._a;
                }

                if (edgeStart != currentInt2)
                    continue;

                currentY = shareEdge._y;
                nextInt2 = edgeEnd;
                findEdge = true;
                sharedVisited[idx] = true;
            }
        }

        // 공유 엣지에서 찾기 실패, Region 엣지에서 찾기 시도
        if (findEdge == false)
        {
            auto range = regionEdgeMap.equal_range(currentInt2);
            for (auto& it = range.first; it != range.second; ++it)
            {
                int idx = it->second;
                if (regionVisited[idx])
                    continue;

                const ContourEdge& regionEdge = regionEdges[idx];

                if (regionEdge._a != currentInt2)
                    continue;

                currentY = regionEdge._y;
                nextInt2 = regionEdge._b;
                findEdge = true;
                regionVisited[idx] = true;
            }
        }

        if (findEdge == false)
        {
            break;
        }

        ContourVertex current;
        current.x = currentInt2.x;
        current.z = currentInt2.z;
        current.y = currentY;

        loop.push_back(current);

        currentInt2 = nextInt2;
    }
    return loop;
}

float Contours::PointToSegmentDist(ContourVertex p, ContourVertex a, ContourVertex b)
{
    float abx = (float)(b.x - a.x);
    float aby = (float)(b.y - a.y);
    float abz = (float)(b.z - a.z);
    float apx = (float)(p.x - a.x);
    float apy = (float)(p.y - a.y);
    float apz = (float)(p.z - a.z);

    float ab2 = abx * abx + aby * aby + abz * abz;

    if (ab2 < 1e-6f)
        return apx * apx + apy * apy + apz * apz;

    float t = (apx * abx + apy * aby + apz * abz) / ab2;
    t = std::clamp(t, 0.0f, 1.0f);

    float dx = apx - t * abx;
    float dy = apy - t * aby;
    float dz = apz - t * abz;

    return dx * dx + dy * dy + dz * dz;
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
        _polyMeshs.push_back(TriangulateEarClipping(contour));
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
