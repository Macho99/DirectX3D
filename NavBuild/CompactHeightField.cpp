#include "pch.h"
#include "CompactHeightField.h"

CompactHeightField::CompactHeightField(const HeightField& heightField, float agentHeight, float agentMaxClimb)
    : HeightFieldBase(heightField)
{
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
            span.connections[0] = UINT16_MAX;
            span.connections[1] = UINT16_MAX;
            span.connections[2] = UINT16_MAX;
            span.connections[3] = UINT16_MAX;

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
                    uint16 neighborSpanIdx = neighborCell.index + j;
                    CompactSpan& neighborSpan = _spans[neighborSpanIdx];
                    int bot = max(span.y, neighborSpan.y);
                    int top = min(span.h, neighborSpan.h);
                    if (top - bot <= agentCellHeight)
                        continue;

                    if (std::abs(span.y - neighborSpan.y) <= agentMaxClimbCell)
                    {
                        span.connections[d] = static_cast<uint16>(neighborSpanIdx);
                        break;
                    }
                }
            }
        }
    }

    BuildRegions();
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
            if (nei == UINT16_MAX)
                continue;

            CompactSpan& neighborSpan = _spans[nei];
            if (neighborSpan.region != 0)
                continue;

            neighborSpan.region = regionId;
            q.push(nei);
        }
    }
}
