#include "pch.h"
#include "PolyMeshField.h"
#include "Contours.h"
#include "NavFileUtils.h"

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
    BuildAdjacentInfo();
    BuildCentroid();
}

PolyMeshField::PolyMeshField(const HeightFieldBase& heightFieldBase, NavFileUtils& fileUtils)
    : HeightFieldBase(heightFieldBase)
{
    LoadFromFile(fileUtils);
}

void PolyMeshField::SaveToFile(NavFileUtils& fileUtils) const
{
    fileUtils.Write<uint32>(polyMeshs.size());
    for (const PolyMesh& polyMesh : polyMeshs)
    {
        fileUtils.Write<uint32>(polyMesh.vertices.size());
        fileUtils.Write(polyMesh.vertices.data(), sizeof(Vertex) * polyMesh.vertices.size());

        fileUtils.Write<uint32>(polyMesh.polys.size());
        for (const Poly& poly : polyMesh.polys)
        {
            fileUtils.Write<uint32>(poly.vertCount);
            for (int i = 0; i < poly.vertCount; ++i)
            {
                fileUtils.Write(poly.indices[i]);
                fileUtils.Write(poly.neighbors[i]);
            }
            fileUtils.Write(poly.centroid);
        }

        fileUtils.Write<uint32>(polyMesh.failIndices.size());
        fileUtils.Write(polyMesh.failIndices.data(), sizeof(int) * polyMesh.failIndices.size());
    }
}

void PolyMeshField::LoadFromFile(NavFileUtils& fileUtils)
{
    polyMeshs.clear();
    uint32 meshCount = fileUtils.Read<uint32>();
    for (uint32 meshIdx = 0; meshIdx < meshCount; meshIdx++)
    {
        PolyMesh polyMesh;
        uint32 vertCount = fileUtils.Read<uint32>();
        polyMesh.vertices.resize(vertCount);
        void* vertDataPtr = polyMesh.vertices.data();
        fileUtils.Read(&vertDataPtr, sizeof(Vertex) * vertCount);

        uint32 polyCount = fileUtils.Read<uint32>();
        polyMesh.polys.resize(polyCount);
        for (uint32 polyIdx = 0; polyIdx < polyCount; polyIdx++)
        {
            Poly& poly = polyMesh.polys[polyIdx];
            poly.vertCount = fileUtils.Read<uint32>();
            for (int vertIdx = 0; vertIdx < poly.vertCount; vertIdx++)
            {
                fileUtils.Read(poly.indices[vertIdx]);
                fileUtils.Read(poly.neighbors[vertIdx]);
            }
            fileUtils.Read(poly.centroid);
        }

        uint32 failIndexCount = fileUtils.Read<uint32>();
        polyMesh.failIndices.resize(failIndexCount);
        void* failIndexDataPtr = polyMesh.failIndices.data();
        fileUtils.Read(&failIndexDataPtr, sizeof(int) * failIndexCount);

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

void PolyMeshField::BuildAdjacentInfo()
{
    unordered_map<pair<Vertex, Vertex>, pair<PolyRef, int>, VertexPairHash> edgeToPoly;

    for (int regionIdx = 0; regionIdx < polyMeshs.size(); ++regionIdx)
    {
        PolyMesh& polyMesh = polyMeshs[regionIdx];
        const vector<Vertex>& vertices = polyMesh.vertices;
        for (int polyIdx = 0; polyIdx < polyMesh.polys.size(); ++polyIdx)
        {
            Poly& poly = polyMesh.polys[polyIdx];
            for (int i = 0; i < poly.vertCount; ++i)
            {
                int idxA = poly.indices[i];
                int idxB = poly.indices[(i + 1) % poly.vertCount];
                Vertex vA = vertices[idxA];
                Vertex vB = vertices[idxB];

                if (vA < vB)
                    std::swap(vA, vB);
                pair<Vertex, Vertex> edgeKey{ vA, vB };
                auto it = edgeToPoly.find(edgeKey);
                if (it == edgeToPoly.end())
                {
                    edgeToPoly[edgeKey] = make_pair(PolyRef{ regionIdx, polyIdx }, i);
                }
                else
                {
                    PolyRef adjPolyInfo = it->second.first;
                    int adjEdgeIdx = it->second.second;
                    Poly& adjacentPoly = polyMeshs[adjPolyInfo.regionIndex].polys[adjPolyInfo.polyIndex];

                    adjacentPoly.neighbors[adjEdgeIdx] = PolyRef{ regionIdx, polyIdx };
                    poly.neighbors[i] = adjPolyInfo;
                }
            }
        }
    }
}

void PolyMeshField::BuildCentroid()
{
    for (int regionIdx = 0; regionIdx < polyMeshs.size(); ++regionIdx)
    {
        PolyMesh& polyMesh = polyMeshs[regionIdx];
        const vector<Vertex>& vertices = polyMesh.vertices;
        for (int polyIdx = 0; polyIdx < polyMesh.polys.size(); ++polyIdx)
        {
            Poly& poly = polyMesh.polys[polyIdx];
            poly.centroid = Vec3(0.f);
            for (int i = 0; i < poly.vertCount; ++i)
            {
                int idx = poly.indices[i];
                const Vertex& v = vertices[idx];
                poly.centroid += v.ToVec3();
            }
            poly.centroid /= poly.vertCount;
        }
    }
}

const Poly& PolyMeshField::GetPoly(const PolyRef& ref) const
{
    if (!ref.IsValid())
        throw std::runtime_error("Invalid PolyRef");
    return polyMeshs[ref.regionIndex].polys[ref.polyIndex];
}

Poly& PolyMeshField::GetPoly(const PolyRef& ref)
{
    return const_cast<Poly&>(static_cast<const PolyMeshField*>(this)->GetPoly(ref));
}

PolyRef PolyMeshField::FindClosestPolyAndPoint(const Vec3& srcPoint, OUT Vec3& closestPoint) const
{
    PolyRef closestRef;
    float closestDistSq = FLT_MAX;
    closestPoint = srcPoint;

    for (int regionIdx = 0; regionIdx < polyMeshs.size(); ++regionIdx)
    {
        const PolyMesh& polyMesh = polyMeshs[regionIdx];
        for (int polyIdx = 0; polyIdx < polyMesh.polys.size(); ++polyIdx)
        {
            const PolyRef polyRef(regionIdx, polyIdx);
            const Vec3 candidate = FindClosestPointInPoly(srcPoint, polyRef);

            // Nav coordinates use different scales for XZ and Y. Compare the
            // candidates after converting the delta to world-space units.
            const Vec3 delta = candidate - srcPoint;
            const float dx = delta.x * _cs;
            const float dy = delta.y * _ch;
            const float dz = delta.z * _cs;
            const float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < closestDistSq)
            {
                closestDistSq = distSq;
                closestRef = polyRef;
                closestPoint = candidate;
            }
        }
    }

    return closestRef;
}

Vec3 PolyMeshField::FindClosestPointInPoly(const Vec3& point, const PolyRef& polyRef) const
{
    const Poly& poly = GetPoly(polyRef);
    const vector<Vertex>& verts = polyMeshs[polyRef.regionIndex].vertices;

    Vec3 closestPoint = point;
    if (IsPointInPoly(point, verts, poly) == false)
    {
        float closestDistSqXZ = FLT_MAX;
        for (int i = 0; i < poly.vertCount; ++i)
        {
            const Vec3 edgeStart = verts[poly.indices[i]].ToVec3();
            const Vec3 edgeEnd = verts[poly.indices[(i + 1) % poly.vertCount]].ToVec3();
            const float edgeX = edgeEnd.x - edgeStart.x;
            const float edgeZ = edgeEnd.z - edgeStart.z;
            const float edgeLengthSqXZ = edgeX * edgeX + edgeZ * edgeZ;
            if (edgeLengthSqXZ < kEps)
                continue;

            const float pointX = point.x - edgeStart.x;
            const float pointZ = point.z - edgeStart.z;
            const float t = std::clamp(
                (pointX * edgeX + pointZ * edgeZ) / edgeLengthSqXZ,
                0.f,
                1.f);
            const Vec3 projectedPoint = edgeStart + (edgeEnd - edgeStart) * t;
            const float dx = projectedPoint.x - point.x;
            const float dz = projectedPoint.z - point.z;
            const float distSqXZ = dx * dx + dz * dz;
            if (distSqXZ < closestDistSqXZ)
            {
                closestDistSqXZ = distSqXZ;
                closestPoint = projectedPoint;
            }
        }
    }

    // A convex PolyMesh polygon is triangulated as a fan only for sampling its
    // surface height. The XZ position above remains the actual closest point.
    for (int i = 1; i + 1 < poly.vertCount; ++i)
    {
        const Vec3 a = verts[poly.indices[0]].ToVec3();
        const Vec3 b = verts[poly.indices[i]].ToVec3();
        const Vec3 c = verts[poly.indices[i + 1]].ToVec3();
        if (PointInTri2D(closestPoint, a, b, c) == false)
            continue;

        const Vec3 normal = (b - a).Cross(c - a);
        if (std::abs(normal.y) <= kEps)
            continue;

        closestPoint.y = GetTriY(closestPoint.x, closestPoint.z, a, b, c);
        return closestPoint;
    }

    // Degenerate polygons should not normally reach this path. Preserve a
    // stable surface Y by taking the height of the nearest XZ edge.
    float closestEdgeDistSqXZ = FLT_MAX;
    for (int i = 0; i < poly.vertCount; ++i)
    {
        const Vec3 edgeStart = verts[poly.indices[i]].ToVec3();
        const Vec3 edgeEnd = verts[poly.indices[(i + 1) % poly.vertCount]].ToVec3();
        const float edgeX = edgeEnd.x - edgeStart.x;
        const float edgeZ = edgeEnd.z - edgeStart.z;
        const float edgeLengthSqXZ = edgeX * edgeX + edgeZ * edgeZ;
        if (edgeLengthSqXZ < kEps)
            continue;

        const float t = std::clamp(
            ((closestPoint.x - edgeStart.x) * edgeX +
             (closestPoint.z - edgeStart.z) * edgeZ) / edgeLengthSqXZ,
            0.f,
            1.f);
        const Vec3 edgePoint = edgeStart + (edgeEnd - edgeStart) * t;
        const float dx = edgePoint.x - closestPoint.x;
        const float dz = edgePoint.z - closestPoint.z;
        const float distSqXZ = dx * dx + dz * dz;
        if (distSqXZ < closestEdgeDistSqXZ)
        {
            closestEdgeDistSqXZ = distSqXZ;
            closestPoint.y = edgePoint.y;
        }
    }

    return closestPoint;
}

bool PolyMeshField::IsPointInPolyRef(const Vec3& point, const PolyRef& polyRef) const
{
    const Poly& poly = GetPoly(polyRef);
    const vector<Vertex>& verts = polyMeshs[polyRef.regionIndex].vertices;
    return IsPointInPoly(point, verts, poly);
}
