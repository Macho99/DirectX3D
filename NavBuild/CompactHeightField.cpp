#include "pch.h"
#include "CompactHeightField.h"

CompactHeightField::CompactHeightField(const HeightField& heightField, const NavBuildSettings& setting)
    : HeightFieldBase(heightField)
{
    const float agentHeight = setting.agentHeight;
    const float agentMaxClimb = setting.agentMaxClimb;
    const int minRegionCount = setting.minRegionCount;
    _cells.resize(_width * _depth);

    const int agentCellHeight = (int)std::ceil(agentHeight / _ch);
    const int agentMaxClimbCell = (int)std::ceil(agentMaxClimb / _ch);
    const vector<vector<Span>>& columns = heightField.GetColumns();
    for (int columnIdx = 0; columnIdx < columns.size(); columnIdx++)
    {
        const vector<Span> column = columns[columnIdx];
        _cells[columnIdx].index = _spans.size();
        _cells[columnIdx].count = 0;

        for (size_t i = 0; i < column.size(); ++i)
        {
            const Span& span = column[i];
            if (span.area == 0)
                continue;
            CompactSpan compactSpan;
            compactSpan.y = span.cmaxY;

            uint16 ceiling = UINT16_MAX;
            if (i + 1 < column.size())
                ceiling = column[i + 1].cminY;
            compactSpan.h = ceiling;

            _spans.push_back(compactSpan);
            _cells[columnIdx].count++;
        }
    }

    for (int cellIdx = 0; cellIdx < _cells.size(); cellIdx++)
    {
        int cx = cellIdx % _width;
        int cz = cellIdx / _width;

        CompactCell& cell = _cells[cellIdx];
        for (int i = 0; i < cell.count; ++i)
        {
            CompactSpan& span = _spans[cell.index + i];
            span.connections[0] = NOT_CONNECTED;
            span.connections[1] = NOT_CONNECTED;
            span.connections[2] = NOT_CONNECTED;
            span.connections[3] = NOT_CONNECTED;

            for (int d = 0; d < 4; d++)
            {
                int ncx = cx + _dx[d];
                int ncz = cz + _dz[d];
                if (ncx < 0 || ncx >= _width || ncz < 0 || ncz >= _depth)
                    continue;

                int neighborCellIdx = GetColumnIndex(ncx, ncz);
                CompactCell& neighborCell = _cells[neighborCellIdx];
                for (int j = 0; j < neighborCell.count; ++j)
                {
                    uint32 neighborSpanIdx = neighborCell.index + j;
                    CompactSpan& neighborSpan = _spans[neighborSpanIdx];
                    int bot = max(span.y, neighborSpan.y);
                    int top = min(span.h, neighborSpan.h);
                    if (top - bot <= agentCellHeight)
                        continue;

                    if (std::abs(span.y - neighborSpan.y) <= agentMaxClimbCell)
                    {
                        span.connections[d] = neighborSpanIdx;
                        break;
                    }
                }
            }
        }
    }

    BuildRegions();
    FilterRegions(minRegionCount);
}

void CompactHeightField::BuildRegions()
{
    _regions.clear();
    _regions.push_back(0); // 0번은 버림
    for (int i = 0; i < _spans.size(); ++i)
    {
        CompactSpan& span = _spans[i];
        if (span.region != 0)
            continue;

        _regions.push_back(0);
        FloodFillRegion(i, _regions.size() - 1);
    }
}

void CompactHeightField::FloodFillRegion(int startIndex, int regionId)
{
    queue<int> q;
    _spans[startIndex].region = regionId;
    q.push(startIndex);

    while (!q.empty())
    {
        int cur = q.front();
        q.pop();
        _regions[regionId]++;

        for (int dir = 0; dir < 4; ++dir)
        {
            int nei = _spans[cur].connections[dir];
            if (nei == NOT_CONNECTED)
                continue;

            CompactSpan& neighborSpan = _spans[nei];
            if (neighborSpan.region != 0)
                continue;

            neighborSpan.region = regionId;
            q.push(nei);
        }
    }
}

void CompactHeightField::FilterRegions(int minRegionSize)
{
    unordered_map<int, int> regionIdToNewId;
    int newRegionId = 1;
    for (int i = 1; i < _regions.size(); ++i)
    {
        if (_regions[i] >= minRegionSize)
        {
            regionIdToNewId[i] = newRegionId;
            newRegionId++;
        }
    }

    for (CompactSpan& span : _spans)
    {
        if (span.region == 0)
            continue;
        auto it = regionIdToNewId.find(span.region);
        if (it != regionIdToNewId.end())
        {
            span.region = it->second;
        }
        else
        {
            span.region = 0;
        }
    }
    vector<int> newRegions(newRegionId, 0);
    for (int i = 1; i < _regions.size(); ++i)
    {
        auto it = regionIdToNewId.find(i);
        if (it != regionIdToNewId.end())
        {
            newRegions[it->second] = _regions[i];
        }
    }
    _regions = std::move(newRegions);
}

vector<vector<vector<ContourVertex>>> CompactHeightField::BuildContours(float maxStepHeight)
{
    vector<vector<vector<ContourVertex>>> contours;
    int maxStepCHeight = (int)std::ceil(maxStepHeight / _ch);
    for (int i = 1; i < _regions.size(); ++i)
    {
        vector<ContourEdge> edges = CollectRegionEdges(i);
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
        contours.push_back(std::move(loops));
    }
    return contours;
}

vector<ContourEdge> CompactHeightField::CollectRegionEdges(uint16 targetRegion)
{
    vector<ContourEdge> edges;

    for (int cz = 0; cz < _depth; cz++)
    {
        for (int cx = 0; cx < _width; cx++)
        {
            const CompactCell& cell = _cells[GetColumnIndex(cx, cz)];
            for (int i = 0; i < cell.count; ++i)
            {
                int spanIdx = cell.index + i;
                const CompactSpan& s = _spans[spanIdx];
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

vector<ContourVertex> CompactHeightField::BuildOneLoop(const vector<ContourEdge>& edges, const EdgeMap& edgeMap, vector<bool>& used, 
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

    while (!(current.x == startPoint.x && current.z == startPoint.z && (std::abs(startPoint.y - current.y) < maxStepCHeight)))
    {
        int nextIdx = -1;

        Int2 currentInt2{ current.x, current.z };
        auto range = edgeMap.equal_range(currentInt2);
        for (auto& it = range.first; it != range.second; ++it)
        {
            int idx = it->second;
            if (used[idx])
                continue;
            if (edges[idx]._region != start._region)
                continue;
            int yDiff = std::abs(edges[idx]._y - start._y);
            if (yDiff > maxStepCHeight)
                continue;

            nextIdx = idx;
            break;
        }

        if (nextIdx < 0)
        {
            // loop가 안 닫힘: 에러 또는 데이터 이상
            break;
        }

        used[nextIdx] = true;
        const ContourEdge& e = edges[nextIdx];

        ContourVertex newCurrent;
        newCurrent.x = e._b.x;
        newCurrent.y = e._y;
        newCurrent.z = e._b.z;
        current = newCurrent;

        loop.push_back(current);
    }

    loop.push_back(startPoint);
    return loop;
}