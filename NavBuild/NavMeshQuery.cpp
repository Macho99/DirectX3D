#include "pch.h"
#include "NavMeshQuery.h"

#include "PolyMeshField.h"
#include "DetailMeshField.h"

NavMeshQuery::NavMeshQuery(const PolyMeshField& polyMeshField, const DetailMeshField& detailMeshField)
    :_polyMeshField(polyMeshField), _detailMeshField(detailMeshField)
{
}

vector<Vec3> NavMeshQuery::FindPath(const Vec3& start, const Vec3& end) const
{
    // 1) 포함 폴리 탐색 (실패 시 가장 가까운 폴리로 폴백)
    //PolyRef startPoly = _polyMeshField.FindContainingPoly(start);
    //if (startPoly.IsValid() == false)
    //    startPoly = _polyMeshField.FindNearestPoly(start);
    //
    //PolyRef goalPoly = _polyMeshField.FindContainingPoly(end);
    //if (goalPoly.IsValid() == false)
    //    goalPoly = _polyMeshField.FindNearestPoly(end);

    PolyRef startPoly = _polyMeshField.FindNearestPoly(start);
    PolyRef goalPoly = _polyMeshField.FindNearestPoly(end);

    if (startPoly.IsValid() == false || goalPoly.IsValid() == false)
        return {};

    // 2) A* → 폴리곤 시퀀스
    vector<PolyPath> polyPathes = FindPolyPath(startPoly, goalPoly);
    if (polyPathes.empty()) return {};

    // 3) 퍼널 → 최종 웨이포인트
    //return FunnelSmooth(polyPath, start, goal);

    vector<Vec3> waypoints;
    waypoints.push_back(start);
    for (const PolyPath& polyPath : polyPathes)
    {
        if (polyPath.index < 0)
            break;

        const Poly& poly = _polyMeshField.GetPoly(polyPath.polyRef);
        const vector<Vertex>& vertices = _polyMeshField.GetPolyMeshs()[polyPath.polyRef.regionIndex].vertices;
        const int edgeIdx = polyPath.index;
        Vertex v0 = vertices[poly.indices[edgeIdx]];
        Vertex v1 = vertices[poly.indices[(edgeIdx + 1) % poly.vertCount]];
        Vec3 edgeMidpoint = (v0.ToVec3() + v1.ToVec3()) * 0.5f;
        waypoints.push_back(edgeMidpoint);
    }
    waypoints.push_back(end);
    return waypoints;
}

vector<PolyPath> NavMeshQuery::FindPolyPath(const PolyRef& startPoly, const PolyRef& goalPoly) const
{
    if (startPoly.IsValid() == false || goalPoly.IsValid() == false)
        return {};
    if (startPoly == goalPoly)
        return { PolyPath(startPoly, -1) };

    const vector<PolyMesh>& polyMeshs = _polyMeshField.GetPolyMeshs();  
    vector<vector<AStarNode>> nodes(polyMeshs.size());
    vector<vector<bool>> closed(polyMeshs.size());
    for (int regionIdx = 0; regionIdx < polyMeshs.size(); ++regionIdx)
    {
        const PolyMesh& polyMesh = polyMeshs[regionIdx];
        nodes[regionIdx].resize(polyMesh.polys.size());
        closed[regionIdx].resize(polyMesh.polys.size(), false);
    }

    auto GetNode = [&](const PolyRef& ref) -> AStarNode&
        { return nodes[ref.regionIndex][ref.polyIndex]; };
    auto GetClosed = [&](const PolyRef& ref) -> bool
        { return closed[ref.regionIndex][ref.polyIndex]; };
    auto SetClosed = [&](const PolyRef& ref, bool value)
        { closed[ref.regionIndex][ref.polyIndex] = value; };

    // 오픈셋: (f값, polyIndex)
    struct Entry
    {
        float f;
        PolyRef polyRef;
        bool operator>(const Entry& other) const
        {
            return f > other.f;
        }
    };
    priority_queue<Entry, vector<Entry>, greater<Entry>> openSet;

    const Vec3 goalCentroid = _polyMeshField.GetPoly(goalPoly).centroid;

    AStarNode& startNode = GetNode(startPoly);
    startNode.g = 0.f;
    startNode.h = (_polyMeshField.GetPoly(startPoly).centroid - goalCentroid).Length();
    openSet.push({ startNode.f(), startPoly });

    while (!openSet.empty())
    {
        Entry curEntry = openSet.top(); openSet.pop();
        const PolyRef& curPolyRef = curEntry.polyRef;
        if (GetClosed(curPolyRef)) continue;
        SetClosed(curPolyRef, true);

        if (curPolyRef == goalPoly)
        {
            // 경로 역추적
            vector<PolyPath> path;
            for (PolyPath curPolyPath = PolyPath(goalPoly, -1); curPolyPath.IsValid(); curPolyPath = GetNode(curPolyPath.polyRef).parent)
                path.push_back(curPolyPath);
            reverse(path.begin(), path.end());
            return path;
        }

        const Poly& poly = _polyMeshField.GetPoly(curPolyRef);
        AStarNode& curNode = GetNode(curPolyRef);
        const Vec3 curCentroid = poly.centroid;

        for (int i = 0; i < poly.vertCount; ++i)
        {
            const PolyRef& next = poly.neighbors[i];
            if (!next.IsValid()) continue;

            if (GetClosed(next))
                continue;

            const Poly& nextPoly = _polyMeshField.GetPoly(next);
            const Vec3 nextCentroid = nextPoly.centroid;
            AStarNode& nextNode = GetNode(next);
            float tentativeG = curNode.g + (curCentroid - nextCentroid).Length();

            if (tentativeG < nextNode.g)
            {
                nextNode.g = tentativeG;
                nextNode.h = (nextCentroid - goalCentroid).Length();
                nextNode.parent = PolyPath(curPolyRef, i);
                openSet.push({ nextNode.f(), next });
            }
        }
    }

    return {}; // 경로 없음
}