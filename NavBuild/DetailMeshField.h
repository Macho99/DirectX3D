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

class NavFileUtils;

class DetailMeshField : public HeightFieldBase
{
public:
    DetailMeshField(const PolyMeshField& polyMeshField, const CompactHeightField& compactHeightField, const NavBuildSettings& settings);
    DetailMeshField(const HeightFieldBase& heightFieldBase, NavFileUtils& fileUtils);

    const vector<DetailMesh>& GetDetailMeshs() const { return _detailMeshs; }
    float SampleHeight(const PolyRef& polyRef, const Vec3& pos) const;
    void SaveToFile(NavFileUtils& fileUtils) const;
    void LoadFromFile(NavFileUtils& fileUtils);

private:
    void SampleEdgeMaxError(const int region, const Vec3& a, const Vec3& b, const CompactHeightField& heightField, float maxError, float stepSize, vector<Vec3>& result);

private:
    vector<DetailMesh> _detailMeshs;
};
