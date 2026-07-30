#pragma once

#include <array>
#include <vector>

namespace Geometry2D
{
    constexpr float DefaultEpsilon = 1e-6f;

    struct Point
    {
        float x = 0.0f;
        float y = 0.0f;

        Point() = default;
        Point(float xValue, float yValue) : x(xValue), y(yValue) {}
    };

    struct TriangleIndices
    {
        std::array<int, 3> indices = {};

        TriangleIndices() = default;
        TriangleIndices(int i0, int i1, int i2) : indices{ i0, i1, i2 } {}
    };

    struct BarycentricCoordinates
    {
        float a = 0.0f;
        float b = 0.0f;
        float c = 0.0f;

        bool IsInside(float epsilon = DefaultEpsilon) const
        {
            return a >= -epsilon && b >= -epsilon && c >= -epsilon;
        }
    };

    inline Point operator+(const Point& lhs, const Point& rhs)
    {
        return { lhs.x + rhs.x, lhs.y + rhs.y };
    }

    inline Point operator-(const Point& lhs, const Point& rhs)
    {
        return { lhs.x - rhs.x, lhs.y - rhs.y };
    }

    inline Point operator*(const Point& point, float scalar)
    {
        return { point.x * scalar, point.y * scalar };
    }

    inline float Dot(const Point& lhs, const Point& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y;
    }

    inline float LengthSquared(const Point& point)
    {
        return Dot(point, point);
    }

    inline float Cross(const Point& a, const Point& b, const Point& c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    bool TryGetBarycentricCoordinates(
        const Point& point,
        const Point& a,
        const Point& b,
        const Point& c,
        BarycentricCoordinates& result,
        float epsilon = DefaultEpsilon);

    bool ContainsPoint(
        const Point& point,
        const Point& a,
        const Point& b,
        const Point& c,
        float epsilon = DefaultEpsilon);

    Point ClosestPointOnSegment(const Point& point, const Point& a, const Point& b);

    Point ClosestPointOnTriangle(
        const Point& point,
        const Point& a,
        const Point& b,
        const Point& c);

    bool IsPointInCircumcircle(
        const Point& point,
        const Point& a,
        const Point& b,
        const Point& c,
        float epsilon = DefaultEpsilon);

    // Bowyer-Watson Delaunay triangulation. Returned indices refer to the input
    // point array. Degenerate and duplicate points are ignored.
    std::vector<TriangleIndices> DelaunayTriangulate(
        const std::vector<Point>& points,
        const std::vector<int>& sourceIndices,
        float epsilon = DefaultEpsilon);

    std::vector<TriangleIndices> DelaunayTriangulate(
        const std::vector<Point>& points,
        float epsilon = DefaultEpsilon);
}
