#include "pch.h"
#include "NavMeshQuery.h"

#include "PolyMeshField.h"
#include "DetailMeshField.h"

NavMeshQuery::NavMeshQuery(const PolyMeshField& polyMeshField, const DetailMeshField& detailMeshField)
    :_polyMeshField(polyMeshField), _detailMeshField(detailMeshField)
{
}

bool NavMeshQuery::TryFindPath(const Vec3& start, const Vec3& end, OUT NavPath& navPath) const
{
    Vec3 closestStart, closestEnd;
    PolyRef startPoly = _polyMeshField.FindClosestPolyAndPoint(start, OUT closestStart);
    PolyRef goalPoly = _polyMeshField.FindClosestPolyAndPoint(end, OUT closestEnd);

    if (startPoly.IsValid() == false || goalPoly.IsValid() == false)
        return false;

    // 2) A* → 폴리곤 시퀀스
    vector<PolyPath> polyPathes = FindPolyPath(startPoly, goalPoly, closestStart, closestEnd);
    if (polyPathes.empty())
        return false;

    navPath.edgeCenterPath.push_back(closestStart);
    for (const PolyPath& polyPath : polyPathes)
    {
        if (polyPath.portalEdgeIndex < 0)
            break;

        const Poly& poly = _polyMeshField.GetPoly(polyPath.polyRef);
        const vector<Vertex>& vertices = _polyMeshField.GetPolyMeshs()[polyPath.polyRef.regionIndex].vertices;
        const int edgeIdx = polyPath.portalEdgeIndex;
        Vertex v0 = vertices[poly.indices[edgeIdx]];
        Vertex v1 = vertices[poly.indices[(edgeIdx + 1) % poly.vertCount]];
        Vec3 edgeMidpoint = (v0.ToVec3() + v1.ToVec3()) * 0.5f;
        navPath.edgeCenterPath.push_back(edgeMidpoint);
    }
    navPath.edgeCenterPath.push_back(end);

    // 3) 퍼널 → 최종 웨이포인트
    navPath.path = FunnelSmooth(polyPathes, closestStart, end);
    return true;
}

vector<PolyPath> NavMeshQuery::FindPolyPath(const PolyRef& startPoly, const PolyRef& goalPoly, const Vec3& startPos, const Vec3& goalPos) const
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

    AStarNode& startNode = GetNode(startPoly);
    startNode.g = 0.f;
    startNode.h = (startPos - goalPos).Length();
    startNode.enterPoint = startPos;
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
        const Vec3& enterPoint = curNode.enterPoint;
        const vector<Vertex>& vertices = _polyMeshField.GetPolyMeshs()[curPolyRef.regionIndex].vertices;

        for (int i = 0; i < poly.vertCount; ++i)
        {
            const PolyRef& next = poly.neighbors[i];
            if (!next.IsValid()) continue;

            if (GetClosed(next))
                continue;

            const Vertex& v0 = vertices[poly.indices[i]];
            const Vertex& v1 = vertices[poly.indices[(i + 1) % poly.vertCount]];
            const Vec3 edgeMidPoint = (v0.ToVec3() + v1.ToVec3()) * 0.5f;

            const Poly& nextPoly = _polyMeshField.GetPoly(next);
            AStarNode& nextNode = GetNode(next);
            float tentativeG = curNode.g + (enterPoint - edgeMidPoint).Length();

            if (tentativeG < nextNode.g)
            {
                nextNode.g = tentativeG;
                nextNode.h = (edgeMidPoint - goalPos).Length();
                nextNode.parent = PolyPath(curPolyRef, i);
                nextNode.enterPoint = edgeMidPoint;
                openSet.push({ nextNode.f(), next });
            }
        }
    }

    return {}; // 경로 없음
}

vector<Vec3> NavMeshQuery::FunnelSmooth(const vector<PolyPath>& polyPath, const Vec3& startPos, const Vec3& endPos) const
{
    if (polyPath.empty()) return {};
    if (polyPath.size() == 1) return { startPos, endPos };

    // 포털 목록 수집
    struct Portal { Vec3 left, right; };
    vector<Portal> portals;
    portals.push_back({ startPos, startPos }); // 시작 더미

    for (int i = 0; i + 1 < (int)polyPath.size(); ++i)
    {
        const PolyPath& path = polyPath[i];
        const Poly& poly = _polyMeshField.GetPoly(polyPath[i].polyRef);
        const vector<Vertex>& vertices = _polyMeshField.GetPolyMeshs()[polyPath[i].polyRef.regionIndex].vertices;
        
        Portal p;
        int leftIdx = poly.indices[path.portalEdgeIndex];
        int rightIdx = poly.indices[(path.portalEdgeIndex + 1) % poly.vertCount];
        p.left = vertices[leftIdx].ToVec3();
        p.right = vertices[rightIdx].ToVec3();
        portals.push_back(p);
    }
    portals.push_back({ endPos, endPos }); // 목표 더미

    // --- Simple Stupid Funnel Algorithm ---
    vector<Vec3> waypoints;
    waypoints.push_back(startPos);

    Vec3 apex = startPos;
    Vec3 left = portals[0].left;
    Vec3 right = portals[0].right;
    int apexIdx = 0, leftIdx = 0, rightIdx = 0;

    for (int i = 1; i < (int)portals.size(); ++i)
    {
        const Vec3& newLeft = portals[i].left;
        const Vec3& newRight = portals[i].right;

        // --- 오른쪽 업데이트 ---
        if (_polyMeshField.Cross2D(apex, right, newRight) <= 0.f)
        {
            if (apex == right || _polyMeshField.Cross2D(apex, left, newRight) > 0.f)
            {
                right = newRight;
                rightIdx = i;
            }
            else
            {
                // 왼쪽 엣지를 넘었음 → 왼쪽 꼭짓점이 새 apex
                waypoints.push_back(left);
                apex = left;
                apexIdx = leftIdx;
                right = apex;
                rightIdx = apexIdx;
                // 포털을 apex 이후부터 재시작
                i = apexIdx;
                continue;
            }
        }

        // --- 왼쪽 업데이트 ---
        if (_polyMeshField.Cross2D(apex, left, newLeft) >= 0.f)
        {
            if (apex == left || _polyMeshField.Cross2D(apex, right, newLeft) < 0.f)
            {
                left = newLeft;
                leftIdx = i;
            }
            else
            {
                waypoints.push_back(right);
                apex = right;
                apexIdx = rightIdx;
                left = apex;
                leftIdx = apexIdx;
                i = apexIdx;
                continue;
            }
        }
    }

    waypoints.push_back(endPos);
    return waypoints;
}