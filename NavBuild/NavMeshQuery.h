#pragma once
class PolyMeshField;
class DetailMeshField;

struct PolyPath
{
    PolyPath() = default;
    PolyPath(const PolyRef& ref, int idx) : polyRef(ref), index(idx) {}

    PolyRef polyRef;
    int index = -1;

    bool operator==(const PolyPath& other) const
    {
        return polyRef == other.polyRef &&
            index == other.index;
    }
    bool operator!=(const PolyPath& other) const
    {
        return !(*this == other);
    }
    bool IsValid() const
    {
        return polyRef.IsValid();
    }
};

struct AStarNode
{
    float g = FLT_MAX;   // 시작점으로부터 비용
    float h = 0.f;       // 휴리스틱 (목표까지 예상 비용)
    float f() const { return g + h; }
    PolyPath parent;
};

class NavMeshQuery
{
public:
    NavMeshQuery(const PolyMeshField& polyMeshField, const DetailMeshField& detailMeshField);

    vector<Vec3> FindPath(const Vec3& start, const Vec3& end) const;

private:
    vector<PolyPath> FindPolyPath(const PolyRef& startPoly, const PolyRef& goalPoly) const;

private:
    const PolyMeshField& _polyMeshField;
    const DetailMeshField& _detailMeshField;
};
