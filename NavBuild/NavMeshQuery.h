#pragma once
class PolyMeshField;
class DetailMeshField;

struct AStarNode
{
    float g = FLT_MAX;   // 시작점으로부터 비용
    float h = 0.f;       // 휴리스틱 (목표까지 예상 비용)
    float f() const { return g + h; }
    PolyPath parent;
    Vec3 enterPoint;
};

class NavMeshQuery
{
public:
    NavMeshQuery(const PolyMeshField& polyMeshField, const DetailMeshField& detailMeshField);

    bool TryFindPath(const Vec3& start, const Vec3& end, MoveInfo& moveInfo) const;
    bool MoveAlongPath(const MoveConfig& config, MoveInfo& moveInfo, float deltaTime) const;

private:
    vector<PolyPath> FindPolyPath(const PolyRef& startPoly, const PolyRef& goalPoly, const Vec3& startPos, const Vec3& goalPos) const;
    vector<Vec3> FunnelSmooth(const vector<PolyPath>& polyPath, const Vec3& startPos, const Vec3& endPos) const;

private:
    const PolyMeshField& _polyMeshField;
    const DetailMeshField& _detailMeshField;
};
