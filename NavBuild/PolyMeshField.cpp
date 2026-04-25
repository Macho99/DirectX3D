#include "pch.h"
#include "PolyMeshField.h"
#include "Contours.h"

PolyMeshField::PolyMeshField(const Contours& contourField)
    : HeightFieldBase(contourField)
{
    vector<vector<ContourVertex>> contours = contourField.GetContours();
    for (const auto& contour : contours)
    {
        PolyMesh polyMesh = TriangulateEarClipping(contour);
        for (int i = 0; i < polyMesh.polys.size(); i++)
        {
            MergeToConvexPolys(polyMesh);
        }
        polyMeshs.push_back(std::move(polyMesh));
    }
}

PolyMesh PolyMeshField::TriangulateEarClipping(const vector<ContourVertex>& verts)
{
    PolyMesh result;
    result.vertices.reserve(verts.size());
    for (const ContourVertex& vertex : verts)
    {
        result.vertices.push_back(vertex);
    }

    if (verts.size() < 3)
        return result;

    vector<int> indices;
    indices.reserve(verts.size());
    for (int i = 0; i < (int)verts.size(); ++i)
        indices.push_back(i);

    int adder = 0;

    bool debug = true;
    while (indices.size() > 3)
    {
        bool foundEar = false;

        adder++;
        float minLen = FLT_MAX;
        int minLenIdx = -1;
        for (int idx = 0; idx < (int)indices.size(); ++idx)
        {
            //int i = (idx + adder) % indices.size();
            int i = idx;

            int i0 = indices[(i - 1 + indices.size()) % indices.size()];
            int i1 = indices[i];
            int i2 = indices[(i + 1) % indices.size()];

            const Vertex& a = verts[i0];
            const Vertex& b = verts[i1];
            const Vertex& c = verts[i2];

            if (!IsConvex(a, b, c))
                continue;

            bool hasPointInside = false;
            for (int j = 0; j < (int)indices.size(); ++j)
            {
                int k = indices[j];
                if (k == i0 || k == i1 || k == i2)
                    continue;

                //int prevK = indices[(j - 1 + indices.size()) % indices.size()];
                //int nextK = indices[(j + 1) % indices.size()];
                //ContourVertex prevP = verts[prevK];
                //ContourVertex nextP = verts[nextK];
                const ContourVertex p = verts[k];
                if (PointInTri2D(p, a, b, c))
                {
                    //int distMinY = std::min({ abs(p.y - a.y), abs(p.y - b.y), abs(p.y - c.y) });
                    //if (distMinY < 10)
                    //{
                    //    Triangle invalidTri{ i0, i1, i2, false };
                    //    result.first.push_back(invalidTri);
                    //    hasPointInside = true;
                    //    break;
                    //}
                    hasPointInside = true;
                    //Triangle invalidTri{ i0, i1, i2, false };
                    //result.first.push_back(invalidTri);
                    break;
                }
            }

            if (hasPointInside)
                continue;

            //float sampledY = SampledAverageY(a, b, c);
            //if (minSampledY >= sampledY)
            //{
            //    minSampledY = sampledY;
            //    minSampledYIdx = i;
            //}
            Vertex2D ac{ c.x - a.x, c.z - a.z };
            int len = ac.LengthSq();
            if (minLen >= len)
            {
                minLen = len;
                minLenIdx = i;
            }
        }

        if (minLenIdx < 0)
        {
            break;
        }

        int i = minLenIdx;
        int i0 = indices[(i - 1 + indices.size()) % indices.size()];
        int i1 = indices[i];
        int i2 = indices[(i + 1) % indices.size()];
        result.polys.push_back(Poly({ i0, i1, i2 }));
        indices.erase(indices.begin() + i);
    }

    if (indices.size() == 3)
        result.polys.push_back(Poly({ indices[0], indices[1], indices[2] }));
    else
    {
        for (int i = 0; i < (int)indices.size(); ++i)
        {
            result.failIndices.push_back(indices[i]);
        }
    }

    return result;
}

bool PolyMeshField::IsConvex(const vector<int>& poly, const vector<Vertex>& verts)
{
    const int polySize = (int)poly.size();
    if (polySize < 3)
        return false;

    for (int i = 0; i < polySize; ++i)
    {
        const Vertex& a = verts[poly[(i - 1 + polySize) % polySize]];
        const Vertex& b = verts[poly[i]];
        const Vertex& c = verts[poly[(i + 1) % polySize]];
        if (Cross2D(a, b, c) < 0)
            return false;
    }

    return true;
}

bool PolyMeshField::PointInTri2D(const Vertex& p, const Vertex& a, const Vertex& b, const Vertex& c)
{
    int c1 = Cross2D(a, b, p);
    int c2 = Cross2D(b, c, p);
    int c3 = Cross2D(c, a, p);

    bool hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0);
    bool hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0);

    return !(hasNeg && hasPos);
}

pair<int, int> PolyMeshField::FindSharedEdge(const Poly& a, const Poly& b)
{
    for (int i = 0; i < a.vertCount; ++i)
    {
        int a0 = a.indices[i];
        int a1 = a.indices[(i + 1) % a.vertCount];

        for (int j = 0; j < b.vertCount; ++j)
        {
            int b0 = b.indices[j];
            int b1 = b.indices[(j + 1) % b.vertCount];

            // 반대 방향으로 공유되어야 함 (winding 일치)
            if (a0 == b1 && a1 == b0)
                return { i, j };
        }
    }
    return { -1, -1 };
}

vector<int> PolyMeshField::BuildMergedVerts(const Poly& a, int edgeA, const Poly& b, int edgeB)
{
    vector<int> verts;
    verts.reserve(a.vertCount + b.vertCount - 2);

    // a에서 shared edge의 끝점(i+1)을 제외하고 순회
    for (int k = 0; k < a.vertCount - 1; ++k)
        verts.push_back(a.indices[(edgeA + 1 + k) % a.vertCount]);

    // b에서 shared edge의 끝점(j+1)을 제외하고 순회
    for (int k = 0; k < b.vertCount - 1; ++k)
        verts.push_back(b.indices[(edgeB + 1 + k) % b.vertCount]);

    return verts;
}

void PolyMeshField::MergeToConvexPolys(PolyMesh& polyMesh)
{
    const vector<Poly>& triangles = polyMesh.polys;
    const vector<Vertex>& positions = polyMesh.vertices;

    int n = static_cast<int>(triangles.size());
    vector<Poly> polys = triangles;   // 작업용 복사본
    vector<bool> merged(n, false);

    bool anyMerged = true;

    // 더 이상 병합이 없을 때까지 반복
    while (anyMerged)
    {
        anyMerged = false;
        vector<Poly> next;
        vector<bool> used(polys.size(), false);

        for (int i = 0; i < (int)polys.size(); ++i)
        {
            if (used[i]) continue;

            Poly current = polys[i];

            for (int j = i + 1; j < (int)polys.size(); ++j)
            {
                if (used[j]) continue;

                // MAX_VERTS 초과 방지
                if (current.vertCount + polys[j].vertCount - 2 > Poly::MAX_VERTS)
                    continue;

                auto [edgeA, edgeB] = FindSharedEdge(current, polys[j]);
                if (edgeA == -1) continue;

                vector<int> merged = BuildMergedVerts(current, edgeA, polys[j], edgeB);

                if (!IsConvex(merged, positions))
                    continue;

                current = Poly(merged);
                used[j] = true;
                anyMerged = true;
            }

            used[i] = true;
            next.push_back(current);
        }

        polys = std::move(next);
    }

    polyMesh.polys = std::move(polys);
}