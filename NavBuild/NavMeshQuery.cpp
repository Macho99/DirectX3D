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
    PolyRef startPoly = FindContainingPoly(mesh, start);
    if (startPoly < 0) startPoly = FindNearestPoly(mesh, start);

    int goalPoly = FindContainingPoly(mesh, goal);
    if (goalPoly < 0) goalPoly = FindNearestPoly(mesh, goal);

    if (startPoly < 0 || goalPoly < 0) return {};

    // 2) A* → 폴리곤 시퀀스
    vector<int> polyPath = FindPolyPath(mesh, startPoly, goalPoly);
    if (polyPath.empty()) return {};

    // 3) 퍼널 → 최종 웨이포인트
    return FunnelSmooth(mesh, polyPath, start, goal);
}
