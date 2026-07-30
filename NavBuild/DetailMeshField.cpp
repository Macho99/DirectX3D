#include "pch.h"
#include "DetailMeshField.h"
#include "PolyMeshField.h"
#include "CompactHeightField.h"
#include "NavFileUtils.h"
#include "../MathLibrary/Geometry2D.h"

namespace
{
    void DelaunayTriangulateXZ(
        const vector<Vec3>& vertices,
        const vector<int>& indices,
        vector<Triangle>& result)
    {
        vector<Geometry2D::Point> points;
        points.reserve(vertices.size());
        for (const Vec3& vertex : vertices)
            points.emplace_back(vertex.x, vertex.z);

        const vector<Geometry2D::TriangleIndices> triangles =
            Geometry2D::DelaunayTriangulate(points, indices);

        result.clear();
        result.reserve(triangles.size());
        for (const Geometry2D::TriangleIndices& triangle : triangles)
        {
            result.emplace_back(
                triangle.indices[0],
                triangle.indices[1],
                triangle.indices[2]);
        }
    }
}

DetailMeshField::DetailMeshField(const PolyMeshField& polyMeshField, const CompactHeightField& compactHeightField, const NavBuildSettings& settings)
    : HeightFieldBase(polyMeshField)
{
    vector<PolyMesh> polyMeshs = polyMeshField.GetPolyMeshs();
    _detailMeshs.resize(polyMeshs.size());

    ParallelFor(1, polyMeshs.size(), [&](int regionIdx)
        {
            const PolyMesh& polyMesh = polyMeshs[regionIdx];
            DetailMesh& detailMesh = _detailMeshs[regionIdx];
            detailMesh.triangles.resize(polyMesh.polys.size());
            vector<Vec3>& detailVertices = detailMesh.vertices;

            for (const Vertex& vertex : polyMesh.vertices)
            {
                detailVertices.push_back(Vec3(vertex.x, vertex.y, vertex.z));
            }

            SharedEdgeCache sharedEdgeCache;
            for (int polyIdx = 0; polyIdx < polyMesh.polys.size(); polyIdx++)
            {
                const Poly& poly = polyMesh.polys[polyIdx];
                vector<Triangle>& curTris = detailMesh.triangles[polyIdx];
                vector<int> detailIndices;
                unordered_set<int> shareIndices;

                for (int i = 0; i < poly.vertCount; i++)
                {
                    int idxA = poly.indices[i];
                    int idxB = poly.indices[(i + 1) % poly.vertCount];
                    shareIndices.insert(idxA);
                    shareIndices.insert(idxB);

                    // 경계 엣지는 Contours에서 Simplify 할 때 샘플링되어 있으므로, 내부 엣지만 최대 오차 샘플링을 수행
                    if (std::abs(idxA - idxB) == 1 || std::abs(idxA - idxB) == polyMesh.vertices.size() - 1)
                        continue;

                    EdgeKey edgeKey(idxA, idxB);
                    auto it = sharedEdgeCache.find(edgeKey);
                    int startIndex, endIndex;
                    if (it == sharedEdgeCache.end())
                    {
                        vector<Vec3> edgeVerts;
                        SampleEdgeMaxError(regionIdx, detailVertices[edgeKey.u], detailVertices[edgeKey.v], compactHeightField, settings.detailSampleMaxError, settings.detailSampleDist, edgeVerts);
                        startIndex = (int)detailVertices.size();
                        detailVertices.insert(detailVertices.end(), edgeVerts.begin(), edgeVerts.end());
                        endIndex = (int)detailVertices.size();
                        sharedEdgeCache[edgeKey] = make_pair(startIndex, endIndex);
                    }
                    else
                    {
                        startIndex = it->second.first;
                        endIndex = it->second.second;
                    }

                    for (int j = startIndex; j < endIndex; j++)
                        detailIndices.push_back(j);
                }

                for (int idx : shareIndices)
                    detailIndices.push_back(idx);

                DelaunayTriangulateXZ(detailMesh.vertices, detailIndices, curTris);

                bool vertexAdded = true;
                while (vertexAdded)
                {
                    vertexAdded = false;

                    float maxErr = 0.0f;
                    Vec3 maxErrPt;
                    for (int triIdx = 0; triIdx < curTris.size(); triIdx++)
                    {
                        const Triangle& tri = curTris[triIdx];
                        Vertex2D boundMin = { INT_MAX, INT_MAX };
                        Vertex2D boundMax = { INT_MIN, INT_MIN };
                        for (int i = 0; i < 3; i++)
                        {
                            int index = tri.indices[i];
                            const Vec3& v = detailVertices[index];
                            boundMin.x = min(boundMin.x, static_cast<int>(v.x));
                            boundMin.z = min(boundMin.z, static_cast<int>(v.z));
                            boundMax.x = max(boundMax.x, static_cast<int>(v.x));
                            boundMax.z = max(boundMax.z, static_cast<int>(v.z));
                        }

                        for (int cx = boundMin.x; cx <= boundMax.x; cx++)
                        {
                            for (int cz = boundMin.z; cz <= boundMax.z; cz++)
                            {
                                float x = static_cast<float>(cx);
                                float z = static_cast<float>(cz);
                                Vec3 p0{ x, 0, z };
                                Vec3 p1{ x + 1, 0, z };
                                Vec3 p2{ x, 0, z + 1 };
                                Vec3 p3{ x + 1, 0, z + 1 };

                                const Vec3& a = detailVertices[tri.indices[0]];
                                const Vec3& b = detailVertices[tri.indices[1]];
                                const Vec3& c = detailVertices[tri.indices[2]];

                                if (PointInTri2D(p0, a, b, c) == false)
                                    continue;
                                if (PointInTri2D(p1, a, b, c) == false)
                                    continue;
                                if (PointInTri2D(p2, a, b, c) == false)
                                    continue;
                                if (PointInTri2D(p3, a, b, c) == false)
                                    continue;

                                int actualHeight;
                                if (compactHeightField.TryGetHeight(cx, cz, regionIdx, OUT actualHeight) == false)
                                    continue;

                                float interpHeight = GetTriY(cx, cz, a, b, c);
                                float err = std::abs(actualHeight - interpHeight);

                                if (err > maxErr)
                                {
                                    maxErr = err;
                                    maxErrPt = { static_cast<float>(cx), static_cast<float>(actualHeight), static_cast<float>(cz) };
                                }
                            }
                        }
                    }

                    if (maxErr > settings.detailSampleMaxError)
                    {
                        bool samePointExists = false;
                        for (const Vec3& v : detailVertices)
                        {
                            if (std::abs(v.x - maxErrPt.x) < kEps && std::abs(v.z - maxErrPt.z) < kEps)
                            {
                                samePointExists = true;
                                break;
                            }
                        }

                        if (samePointExists)
                            break;
                        detailVertices.push_back(maxErrPt);
                        detailIndices.push_back(detailVertices.size() - 1);

                        DelaunayTriangulateXZ(detailMesh.vertices, detailIndices, curTris);
                        vertexAdded = true;
                    }
                }
            }
        });
}

DetailMeshField::DetailMeshField(const HeightFieldBase& heightFieldBase, NavFileUtils& fileUtils)
    :HeightFieldBase(heightFieldBase)
{
    LoadFromFile(fileUtils);
}

float DetailMeshField::SampleHeight(const PolyRef& polyRef, const Vec3& pos) const
{
    if (polyRef.IsValid() == false)
        return pos.y;

    const DetailMesh& detailMesh = _detailMeshs[polyRef.regionIndex];
    const vector<Vec3>& vertices = detailMesh.vertices;
    const vector<Triangle>& triangles = detailMesh.triangles[polyRef.polyIndex];

    for (const Triangle& tri : triangles)
    {
        if (IsPointInTriangle(pos, vertices, tri))
        {
            const Vec3& a = vertices[tri.indices[0]];
            const Vec3& b = vertices[tri.indices[1]];
            const Vec3& c = vertices[tri.indices[2]];
            return GetTriY(pos.x, pos.z, a, b, c);
        }
    }
    return pos.y;
}

void DetailMeshField::SaveToFile(NavFileUtils& fileUtils) const
{
    fileUtils.Write((int)_detailMeshs.size());
    for (const DetailMesh& detailMesh : _detailMeshs)
    {
        fileUtils.Write((int)detailMesh.vertices.size());
        fileUtils.Write(detailMesh.vertices.data(), sizeof(Vec3) * detailMesh.vertices.size());

        fileUtils.Write((int)detailMesh.triangles.size());
        for (const vector<Triangle>& tris : detailMesh.triangles)
        {
            fileUtils.Write((int)tris.size());
            for (const Triangle& tri : tris)
            {
                fileUtils.Write(tri.indices[0]);
                fileUtils.Write(tri.indices[1]);
                fileUtils.Write(tri.indices[2]);
            }
        }
    }
}

void DetailMeshField::LoadFromFile(NavFileUtils& fileUtils)
{
    _detailMeshs.clear();
    int meshCount = fileUtils.Read<int>();
    for (int i = 0; i < meshCount; i++)
    {
        DetailMesh detailMesh;
        int vertCount = fileUtils.Read<int>();
        detailMesh.vertices.resize(vertCount);

        void* vertDataPtr = detailMesh.vertices.data();
        fileUtils.Read(&vertDataPtr, sizeof(Vec3) * vertCount);

        int triGroupCount = fileUtils.Read<int>();
        detailMesh.triangles.resize(triGroupCount);
        for (int j = 0; j < triGroupCount; j++)
        {
            int triCount = fileUtils.Read<int>();
            detailMesh.triangles[j].resize(triCount);
            for (int k = 0; k < triCount; k++)
            {
                Triangle& tri = detailMesh.triangles[j][k];
                tri.indices[0] = fileUtils.Read<int>();
                tri.indices[1] = fileUtils.Read<int>();
                tri.indices[2] = fileUtils.Read<int>();
            }
        }
        _detailMeshs.push_back(std::move(detailMesh));
    }
}

void DetailMeshField::SampleEdgeMaxError(const int region, const Vec3& a, const Vec3& b, const CompactHeightField& heightField, float maxError, float stepSize, vector<Vec3>& result)
{    // A→B 방향 벡터
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < kEps) return;

    // 1칸씩 이동하며 각 점의 오차 계산
    // 오차 = 실제 높이 - A/B 선형 보간 높이
    int steps = (int)(len / stepSize);
    if (steps < 2) return; // 너무 짧으면 분할 불필요

    float maxErr = 0.0f;
    Vec3 maxErrPt;

    for (int i = 1; i < steps; ++i)
    {
        float t = (float)i / (float)steps;
        float wx = a.x + dx * t + kEps;
        float wz = a.z + dz * t + kEps;

        float yInterp = a.y + (b.y - a.y) * t;
        int yActual;
        if(heightField.TryGetHeight(wx, wz, region, OUT yActual) == false)
        {
            continue;
        }

        float err = std::abs(yActual - yInterp);
        if (err > maxErr)
        {
            maxErr = err;
            maxErrPt = { wx, static_cast<float>(yActual), wz };
        }
    }

    // 최대 오차가 임계값 이하 → 이 구간은 선형으로 충분
    if (maxErr <= maxError)
        return;

    // 최대 오차 지점 P를 기준으로 분할 정복
    SampleEdgeMaxError(region, a, maxErrPt, heightField, maxError, stepSize, result);
    result.push_back(maxErrPt);
    SampleEdgeMaxError(region, maxErrPt, b, heightField, maxError, stepSize, result);
}
