#include "pch.h"
#include "Contours.h"
#include "CompactHeightField.h"


Contours::Contours(const CompactHeightField& heightField, const NavBuildSettings& settings)
    : HeightFieldBase(heightField)
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

    loop.push_back(startPoint);
    return loop;
}