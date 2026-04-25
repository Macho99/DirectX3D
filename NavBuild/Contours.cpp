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

/*
void Contours::GreedySimplify(float maxError)
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
*/

void Contours::RDPSimplify(float maxError)
{
    for (auto& loop : _contours)
    {
        if (loop.size() <= 3)
            continue;

        const int loopSize = (int)loop.size();
        vector<bool> keep(loopSize, false);

        // 1단계: 무조건 유지할 정점 마킹 (neighborRegion 전환점)
        for (int i = 0; i < loopSize; i++)
        {
            int prev = (i - 1 + loopSize) % loopSize;
            if (loop[prev].neighborRegion != loop[i].neighborRegion)
                keep[i] = true;
        }

        // 2단계: keep 정점들 사이 구간마다 RDP 적용
        // keep 인덱스 목록 수집
        vector<int> anchors;
        for (int i = 0; i < loopSize; i++)
            if (keep[i]) anchors.push_back(i);

        // keep이 하나도 없으면 임의로 하나 고정
        if (anchors.empty())
        {
            keep[0] = true;
            anchors.push_back(0);
        }

        // 각 구간 [anchor[k] → anchor[k+1]) 에 RDP
        const int anchorSize = (int)anchors.size();
        for (int k = 0; k < anchorSize; k++)
        {
            int si = anchors[k];
            int ei = anchors[(k + 1) % anchorSize];
            RDP(loop, keep, si, ei, maxError);
        }

        // 3단계: keep된 정점만 남기기
        vector<ContourVertex> result;
        result.reserve(loopSize);
        for (int i = 0; i < loopSize; i++)
            if (keep[i]) result.push_back(loop[i]);

        loop = move(result);
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

    unordered_set<ContourVertex, VertexHash> visited;

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

            int visitedCount = visited.count(vertex);
            if (visitedCount > 0)
            {
                while (loop.back() != vertex)
                {
                    loop.pop_back();
                }
            }
            else
            {
                visited.insert(vertex);
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
    Vertex2D vertexInt2;
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

void Contours::RDP(const vector<ContourVertex>& loop, vector<bool>& keep, int si, int ei, float maxError)
{
    const int loopSize = (int)loop.size();

    // si와 ei가 인접하면 중간 구간 없음
    if ((si + 1) % loopSize == ei) return;

    const ContourVertex& va = loop[si];
    const ContourVertex& vb = loop[ei];

    // si~ei 사이에서 최대 편차 점 탐색
    float maxD = 0.0f;
    int   maxI = -1;

    int j = (si + 1) % loopSize;
    while (j != ei)
    {
        constexpr float kEps = 1e-6f;
        float d = PointToSegmentDist(loop[j], va, vb);

        if (d > maxD + kEps)
        {
            maxD = d; 
            maxI = j; 
        }
        else if (maxI != -1 && d > maxD - kEps)
        {
            const ContourVertex& curVertex = loop[j];
            const ContourVertex& maxVertex = loop[maxI];
            const int curX = curVertex.x;
            const int curZ = curVertex.z;
            const int maxX = maxVertex.x;
            const int maxZ = maxVertex.z;
            if (curX < maxX || (curX == maxX && curZ < maxZ))
            {
                maxD = d;
                maxI = j;
            }
        }

        j = (j + 1) % loopSize;
    }

    if (maxD > maxError && maxI != -1)
    {
        // 최대 편차 점 유지 후 재귀
        keep[maxI] = true;
        RDP(loop, keep, si, maxI, maxError);
        RDP(loop, keep, maxI, ei, maxError);
    }
    // maxD <= maxError면 구간 내 모든 점 제거 (keep 안 함)
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

/*bool Contours::PointInTri2D(const ContourVertex& prevP, const ContourVertex& p, const ContourVertex& nextP, const ContourVertex& a, const ContourVertex& b, const ContourVertex& c)
{
    int c1 = Cross2D(a, b, p);
    int c2 = Cross2D(b, c, p);
    int c3 = Cross2D(c, a, p);

    if (c1 == 0 || c2 == 0 || c3 == 0)
    {
        Int2 pVec{ p.x - prevP.x, p.z - prevP.z };
        Int2 nextPVec{ nextP.x - p.x, nextP.z - p.z }; 
    
        Int2 edgeVec;
        if (c1 == 0)
        {
            edgeVec = Int2{ b.x - a.x, b.z - a.z };
        }
        else if (c2 == 0)
        {
            edgeVec = Int2{ c.x - b.x, c.z - b.z };
        }
        else if (c3 == 0)
        {
            edgeVec = Int2{ a.x - c.x, a.z - c.z };
        }
    
        int dot = edgeVec.Dot(pVec);
        int lenSq1 = edgeVec.Dot(edgeVec);
        int lenSq2 = pVec.Dot(pVec);
        if (dot * dot == lenSq1 * lenSq2 && dot < 0)
        {
            return false;
        }
        dot = edgeVec.Dot(nextPVec);
        lenSq2 = nextPVec.Dot(nextPVec);
        if (dot * dot == lenSq1 * lenSq2 && dot < 0)
        {
            return false;
        }
    }

    bool hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0);
    bool hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0);

    return !(hasNeg && hasPos);
}*/