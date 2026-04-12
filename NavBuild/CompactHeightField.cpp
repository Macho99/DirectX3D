#include "pch.h"
#include "CompactHeightField.h"
#include "HeightField.h"

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
    _dists.resize(_spans.size(), INT_MAX);
    BuildDistances();
    WatershedRegion(setting.debugSeedCount);

    //BuildRegions();
    //FilterRegions(minRegionCount);
}

void CompactHeightField::BuildDistances()
{
    for (int cellIdx = 0; cellIdx < _cells.size(); cellIdx++)
    {
        int cx = cellIdx % _width;
        int cz = cellIdx / _width;

        CompactCell& cell = _cells[cellIdx];
        for (int i = 0; i < cell.count; ++i)
        {
            int spanIdx = cell.index + i;
            CompactSpan& span = _spans[spanIdx];

            if (span.connections[0] == NOT_CONNECTED ||
                span.connections[1] == NOT_CONNECTED ||
                span.connections[2] == NOT_CONNECTED ||
                span.connections[3] == NOT_CONNECTED)
            {
                _dists[spanIdx] = 0;
                continue;
            }

            int upIdx = span.connections[2];
            if (upIdx != NOT_CONNECTED)
                _dists[spanIdx] = std::min(_dists[spanIdx], _dists[upIdx] + 1);

            int leftIdx = span.connections[3];
            if (leftIdx != NOT_CONNECTED)
                _dists[spanIdx] = std::min(_dists[spanIdx], _dists[leftIdx] + 1);
        }
    }

    for (int cellIdx = _cells.size() - 1; cellIdx >= 0; cellIdx--)
    {
        int cx = cellIdx % _width;
        int cz = cellIdx / _width;
        CompactCell& cell = _cells[cellIdx];
        for (int i = cell.count - 1; i >= 0; --i)
        {
            int spanIdx = cell.index + i;
            CompactSpan& span = _spans[spanIdx];

            int downIdx = span.connections[0];
            if (downIdx != NOT_CONNECTED)
                _dists[spanIdx] = std::min(_dists[spanIdx], _dists[downIdx] + 1);

            int rightIdx = span.connections[1];
            if (rightIdx != NOT_CONNECTED)
                _dists[spanIdx] = std::min(_dists[spanIdx], _dists[rightIdx] + 1);
        }
    }

    for (int dist : _dists)
    {
        _maxDist = std::max(_maxDist, dist);
    }
}

void CompactHeightField::WatershedRegion(int debugSeedDist)
{
    _regions.clear();
    _regions.push_back(0); // 0번은 버림

    struct BFSNode
    {
        int spanIdx;
        int region;
    };

    queue<int> floodFillQueue;
    queue<BFSNode> curQueue;
    queue<BFSNode> nextQueue;

    for (int seedDist = _maxDist; seedDist > 0; seedDist--)
    {
        if (debugSeedDist > 0 && seedDist < debugSeedDist)
        {
            break;
        }

        // seed 찾기
        for (int i = 0; i < _spans.size(); ++i)
        {
            CompactSpan& span = _spans[i];
            if (span.region != 0)
                continue;
            const int spanDist = _dists[i];
            if (spanDist != seedDist)
                continue;
            _regions.push_back(0);
            int regionId = (int)_regions.size() - 1;
            span.region = regionId;
            
            floodFillQueue.push(i);
            while (!floodFillQueue.empty())
            {
                int curIdx = floodFillQueue.front();
                floodFillQueue.pop();
                _regions[regionId]++;
                curQueue.push({ curIdx, regionId });

                auto CheckNeibor = [&](int nei)
                    {
                        if (nei == NOT_CONNECTED)
                            return;
                        CompactSpan& neighborSpan = _spans[nei];
                        if (neighborSpan.region != 0)
                            return;
                        const int neiDist = _dists[nei];
                        if (neiDist != seedDist)
                            return;
                        neighborSpan.region = regionId;
                        floodFillQueue.push(nei);
                    };

                for (int dir = 0; dir < 4; ++dir)
                {
                    int nei = _spans[curIdx].connections[dir];
                    CheckNeibor(nei);
                }
                //하, 우, 상, 좌
                CheckNeibor(GetExtraConnection(curIdx, 2, 3));
                CheckNeibor(GetExtraConnection(curIdx, 2, 1));
                CheckNeibor(GetExtraConnection(curIdx, 0, 3));
                CheckNeibor(GetExtraConnection(curIdx, 0, 1));
            }
        }

        const int fillDist = seedDist - 1;
        while (!curQueue.empty())
        {
            BFSNode node = curQueue.front();
            curQueue.pop();
            _regions[node.region]++;
            for (int dir = 0; dir < 4; ++dir)
            {
                int nei = _spans[node.spanIdx].connections[dir];
                if (nei == NOT_CONNECTED)
                    continue;

                CompactSpan& neighborSpan = _spans[nei];
                if (neighborSpan.region != 0)
                    continue;

                if (_dists[nei] == fillDist)
                {
                    neighborSpan.region = node.region;
                    curQueue.push({ nei, node.region });
                }
                else if (_dists[nei] == fillDist - 1)
                {
                    neighborSpan.region = node.region;
                    nextQueue.push({ nei, node.region });
                }
            }
        }

        std::swap(curQueue, nextQueue);
    }
}

int CompactHeightField::GetExtraConnection(int spanIdx, int dirFirst, int dirSecond)
{
    CompactSpan& span = _spans[spanIdx];
    int neiFirst = span.connections[dirFirst];
    if (neiFirst != NOT_CONNECTED)
    {
        CompactSpan& neighborSpanFirst = _spans[neiFirst];
        int neiSecond = neighborSpanFirst.connections[dirSecond];
        if (neiSecond != NOT_CONNECTED)
        {
            return neiSecond;
        }
    }

    int neiSubFirst = span.connections[dirSecond];
    if (neiSubFirst != NOT_CONNECTED)
    {
        CompactSpan& neighborSpanSubFirst = _spans[neiSubFirst];
        int neiSubSecond = neighborSpanSubFirst.connections[dirFirst];
        if (neiSubSecond != NOT_CONNECTED)
        {
            return neiSubSecond;
        }
    }

    return NOT_CONNECTED;
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