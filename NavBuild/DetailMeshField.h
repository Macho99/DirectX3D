#pragma once
#include "HeightFieldBase.h"

class PolyMeshField;
class CompactHeightField;

// 엣지를 방향 없이 식별하는 키
struct EdgeKey
{
    int u, v; // u < v 로 정렬 보관

    EdgeKey(int a, int b)
        : u(min(a, b)), v(max(a, b))
    {
    }

    bool operator==(const EdgeKey& o) const { return u == o.u && v == o.v; }
};

struct EdgeKeyHash
{
    size_t operator()(const EdgeKey& e) const
    {
        return hash<int>()(e.u) ^ (hash<int>()(e.v) << 16);
    }
};

using SharedEdgeCache = unordered_map<EdgeKey, pair<int, int>, EdgeKeyHash>;

struct DetailMesh
{
    vector<Vec3> vertices;
    vector<vector<Triangle>> triangles;
};

class DetailMeshField : public HeightFieldBase
{
public:
    DetailMeshField(const PolyMeshField& polyMeshField, const CompactHeightField& compactHeightField, const NavBuildSettings& settings);

    const vector<DetailMesh>& GetDetailMeshs() const { return _detailMeshs; }

private:
    void SampleEdgeMaxError(const int region, const Vec3& a, const Vec3& b, const CompactHeightField& heightField, float maxError, float stepSize, vector<Vec3>& result);
    bool InCircumcircle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& p);
    void DelaunayTriangulate(vector<Vec3>& verts, const vector<int>& indices, vector<Triangle>& curTris);

private:
    vector<DetailMesh> _detailMeshs;
};