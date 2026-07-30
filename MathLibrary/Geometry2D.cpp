#include "pch.h"
#include "Geometry2D.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
    using Edge = std::pair<int, int>;

    struct EdgeHash
    {
        size_t operator()(const Edge& edge) const noexcept
        {
            const size_t h1 = std::hash<int>{}(edge.first);
            const size_t h2 = std::hash<int>{}(edge.second);
            return h1 ^ (h2 * 2654435761ULL);
        }
    };

    Edge MakeEdge(int a, int b)
    {
        return a < b ? Edge{ a, b } : Edge{ b, a };
    }
}

bool Geometry2D::TryGetBarycentricCoordinates(
    const Point& point,
    const Point& a,
    const Point& b,
    const Point& c,
    BarycentricCoordinates& result,
    float epsilon)
{
    const float denominator =
        (b.y - c.y) * (a.x - c.x) +
        (c.x - b.x) * (a.y - c.y);

    if (std::abs(denominator) <= epsilon)
    {
        result = {};
        return false;
    }

    result.a =
        ((b.y - c.y) * (point.x - c.x) +
         (c.x - b.x) * (point.y - c.y)) / denominator;
    result.b =
        ((c.y - a.y) * (point.x - c.x) +
         (a.x - c.x) * (point.y - c.y)) / denominator;
    result.c = 1.0f - result.a - result.b;
    return true;
}

bool Geometry2D::ContainsPoint(
    const Point& point,
    const Point& a,
    const Point& b,
    const Point& c,
    float epsilon)
{
    BarycentricCoordinates coordinates;
    return TryGetBarycentricCoordinates(point, a, b, c, coordinates, epsilon) &&
        coordinates.IsInside(epsilon);
}

Geometry2D::Point Geometry2D::ClosestPointOnSegment(
    const Point& point,
    const Point& a,
    const Point& b)
{
    const Point ab = b - a;
    const float lengthSquared = LengthSquared(ab);
    if (lengthSquared <= DefaultEpsilon)
        return a;

    const float t = std::clamp(Dot(point - a, ab) / lengthSquared, 0.0f, 1.0f);
    return a + ab * t;
}

Geometry2D::Point Geometry2D::ClosestPointOnTriangle(
    const Point& point,
    const Point& a,
    const Point& b,
    const Point& c)
{
    if (ContainsPoint(point, a, b, c))
        return point;

    const Point candidates[] =
    {
        ClosestPointOnSegment(point, a, b),
        ClosestPointOnSegment(point, b, c),
        ClosestPointOnSegment(point, c, a)
    };

    Point closest = candidates[0];
    float closestDistanceSquared = LengthSquared(point - closest);
    for (int i = 1; i < 3; ++i)
    {
        const float distanceSquared = LengthSquared(point - candidates[i]);
        if (distanceSquared < closestDistanceSquared)
        {
            closest = candidates[i];
            closestDistanceSquared = distanceSquared;
        }
    }
    return closest;
}

bool Geometry2D::IsPointInCircumcircle(
    const Point& point,
    const Point& a,
    const Point& b,
    const Point& c,
    float epsilon)
{
    const double ax = static_cast<double>(a.x) - point.x;
    const double ay = static_cast<double>(a.y) - point.y;
    const double bx = static_cast<double>(b.x) - point.x;
    const double by = static_cast<double>(b.y) - point.y;
    const double cx = static_cast<double>(c.x) - point.x;
    const double cy = static_cast<double>(c.y) - point.y;

    const double determinant =
        ax * (by * (cx * cx + cy * cy) - cy * (bx * bx + by * by)) -
        ay * (bx * (cx * cx + cy * cy) - cx * (bx * bx + by * by)) +
        (ax * ax + ay * ay) * (bx * cy - by * cx);

    const float orientation = Cross(a, b, c);
    if (orientation > epsilon)
        return determinant > epsilon;
    if (orientation < -epsilon)
        return determinant < -epsilon;
    return false;
}

std::vector<Geometry2D::TriangleIndices> Geometry2D::DelaunayTriangulate(
    const std::vector<Point>& points,
    const std::vector<int>& sourceIndices,
    float epsilon)
{
    std::vector<int> indices;
    indices.reserve(sourceIndices.size());

    std::unordered_set<int> seenIndices;
    for (int index : sourceIndices)
    {
        if (index < 0 || index >= static_cast<int>(points.size()))
            continue;
        if (!seenIndices.insert(index).second)
            continue;

        bool duplicatePoint = false;
        for (int acceptedIndex : indices)
        {
            const Point difference = points[index] - points[acceptedIndex];
            if (LengthSquared(difference) <= epsilon * epsilon)
            {
                duplicatePoint = true;
                break;
            }
        }

        if (!duplicatePoint)
            indices.push_back(index);
    }

    if (indices.size() < 3)
        return {};

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    for (int index : indices)
    {
        minX = std::min(minX, points[index].x);
        minY = std::min(minY, points[index].y);
        maxX = std::max(maxX, points[index].x);
        maxY = std::max(maxY, points[index].y);
    }

    std::vector<Point> workPoints = points;
    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    const float extent = std::max({ maxX - minX, maxY - minY, 1.0f });

    const int super0 = static_cast<int>(workPoints.size());
    const int super1 = super0 + 1;
    const int super2 = super0 + 2;
    workPoints.emplace_back(centerX - 20.0f * extent, centerY - extent);
    workPoints.emplace_back(centerX, centerY + 20.0f * extent);
    workPoints.emplace_back(centerX + 20.0f * extent, centerY - extent);

    std::vector<TriangleIndices> triangles;
    triangles.emplace_back(super0, super1, super2);

    for (int newPointIndex : indices)
    {
        std::unordered_map<Edge, int, EdgeHash> edgeCounts;
        std::vector<TriangleIndices> keptTriangles;
        keptTriangles.reserve(triangles.size());

        for (const TriangleIndices& triangle : triangles)
        {
            const Point& a = workPoints[triangle.indices[0]];
            const Point& b = workPoints[triangle.indices[1]];
            const Point& c = workPoints[triangle.indices[2]];

            if (!IsPointInCircumcircle(workPoints[newPointIndex], a, b, c, epsilon))
            {
                keptTriangles.push_back(triangle);
                continue;
            }

            for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
            {
                const int edgeStart = triangle.indices[edgeIndex];
                const int edgeEnd = triangle.indices[(edgeIndex + 1) % 3];
                ++edgeCounts[MakeEdge(edgeStart, edgeEnd)];
            }
        }

        triangles = std::move(keptTriangles);
        for (const auto& [edge, count] : edgeCounts)
        {
            if (count != 1)
                continue;

            int i0 = edge.first;
            int i1 = edge.second;
            const float orientation = Cross(
                workPoints[i0],
                workPoints[i1],
                workPoints[newPointIndex]);
            if (std::abs(orientation) <= epsilon)
                continue;

            // Keep clockwise winding for compatibility with NavBuild.
            if (orientation > 0.0f)
                std::swap(i0, i1);
            triangles.emplace_back(i0, i1, newPointIndex);
        }
    }

    triangles.erase(
        std::remove_if(
            triangles.begin(),
            triangles.end(),
            [super0](const TriangleIndices& triangle)
            {
                return triangle.indices[0] >= super0 ||
                    triangle.indices[1] >= super0 ||
                    triangle.indices[2] >= super0;
            }),
        triangles.end());

    return triangles;
}

std::vector<Geometry2D::TriangleIndices> Geometry2D::DelaunayTriangulate(
    const std::vector<Point>& points,
    float epsilon)
{
    std::vector<int> indices(points.size());
    for (int i = 0; i < static_cast<int>(points.size()); ++i)
        indices[i] = i;
    return DelaunayTriangulate(points, indices, epsilon);
}
