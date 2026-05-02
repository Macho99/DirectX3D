#pragma once
#include "HeightFieldBase.h"

struct PolyMesh
{
    vector<Vertex> vertices;
    vector<Poly> polys;
    vector<int> failIndices;
};

class PolyMeshField : public HeightFieldBase
{
    using Super = HeightFieldBase;
    using Super::IsConvex;
public:
    PolyMeshField() = delete;
    PolyMeshField(const class Contours& contours);
    const vector<PolyMesh>& GetPolyMeshs() const { return polyMeshs; }

public:
    const Poly& GetPoly(const PolyRef& ref) const;
    Poly& GetPoly(const PolyRef& ref);
    PolyRef FindClosestPolyAndPoint(const Vec3 & srcPoint, OUT Vec3& closestPoint) const;
    Vec3 FindClosestPointInPoly(const Vec3& point, const PolyRef& polyRef) const;
    bool IsPointInPoly(const Vec3& point, const PolyRef& polyRef) const;

private:
    PolyMesh TriangulateEarClipping(const vector<ContourVertex>& verts);

    bool IsConvex(const vector<int>& poly, const vector<Vertex>& verts);

    pair<int, int> FindSharedEdge(const Poly& a, const Poly& b);
    vector<int> BuildMergedVerts(const Poly& a, int edgeA, const Poly& b, int edgeB);
    void MergeToConvexPolys(PolyMesh& polyMesh);
    void BuildAdjacentInfo();
    void BuildCentroid();

private:
    vector<PolyMesh> polyMeshs;
};