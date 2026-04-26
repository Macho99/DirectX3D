#include "pch.h"
#include "DetailMeshField.h"
#include "PolyMeshField.h"
#include "CompactHeightField.h"

DetailMeshField::DetailMeshField(const PolyMeshField& polyMeshField, const CompactHeightField& compactHeightField, const NavBuildSettings& settings)
    : HeightFieldBase(polyMeshField)
{
    vector<PolyMesh> polyMeshs = polyMeshField.GetPolyMeshs();
    _detailMeshs.resize(polyMeshs.size());
    for (int regionIdx = 0; regionIdx < polyMeshs.size(); regionIdx++)
    {
        const PolyMesh& polyMesh = polyMeshs[regionIdx];
        DetailMesh& detailMesh = _detailMeshs[regionIdx];

        for (const Vertex& vertex : polyMesh.vertices)
        {
            detailMesh.vertices.push_back(Vec3(vertex.x, vertex.y, vertex.z));
        }

        SharedEdgeCache sharedEdgeCache;
        for (const Poly& poly : polyMesh.polys)
        {
            for (int i = 0; i < poly.vertCount; i++)
            {
                int idxA = poly.indices[i];
                int idxB = poly.indices[(i + 1) % poly.vertCount];

                // 경계 엣지는 Contours에서 Simplify 할 때 샘플링되어 있으므로, 내부 엣지만 최대 오차 샘플링을 수행
                if (std::abs(idxA - idxB) == 1 || std::abs(idxA - idxB) == polyMesh.vertices.size() - 1)
                    continue;

                EdgeKey edgeKey(idxA, idxB);
                auto it = sharedEdgeCache.find(edgeKey);
                if (it == sharedEdgeCache.end())
                {
                    vector<Vec3> edgeVerts;
                    SampleEdgeMaxError(regionIdx, detailMesh.vertices[edgeKey.u], detailMesh.vertices[edgeKey.v], compactHeightField, settings.detailSampleMaxError, settings.detailSampleDist, edgeVerts);
                    sharedEdgeCache[edgeKey] = edgeVerts;
                    detailMesh.vertices.insert(detailMesh.vertices.end(), edgeVerts.begin(), edgeVerts.end());
                }
            }
        }
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

bool DetailMeshField::InCircumcircle(const Vec3& a, const Vec3& b,
    const Vec3& c, const Vec3& p)
{
    double ax = a.x - p.x, az = a.z - p.z;
    double bx = b.x - p.x, bz = b.z - p.z;
    double cx = c.x - p.x, cz = c.z - p.z;

    double det = ax * (bz * (cx * cx + cz * cz) - cz * (bx * bx + bz * bz))
        - az * (bx * (cx * cx + cz * cz) - cx * (bx * bx + bz * bz))
        + (ax * ax + az * az) * (bx * cz - bz * cx);

    return det > 0.0;
}