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

private:
    PolyMesh TriangulateEarClipping(const vector<ContourVertex>& verts);

    bool IsConvex(const vector<int>& poly, const vector<Vertex>& verts);
    bool PointInTri2D(const Vertex& p, const Vertex& a, const Vertex& b, const Vertex& c);

    pair<int, int> FindSharedEdge(const Poly& a, const Poly& b);
    vector<int> BuildMergedVerts(const Poly& a, int edgeA, const Poly& b, int edgeB);
    void MergeToConvexPolys(PolyMesh& polyMesh);

private:
    vector<PolyMesh> polyMeshs;
};

