#pragma once

#pragma once
#include <cmath>
#include <cfloat>

struct Vec3
{
    float x, y, z;

    constexpr Vec3() noexcept : x(0), y(0), z(0) {}
    constexpr explicit Vec3(float s) noexcept : x(s), y(s), z(s) {}
    constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& r) const noexcept { return Vec3(x + r.x, y + r.y, z + r.z); }
    constexpr Vec3 operator-(const Vec3& r) const noexcept { return Vec3(x - r.x, y - r.y, z - r.z); }
    constexpr Vec3 operator*(float s) const noexcept { return Vec3(x * s, y * s, z * s); }
    constexpr Vec3 operator/(float s) const noexcept { return Vec3(x / s, y / s, z / s); }

    Vec3& operator+=(const Vec3& r) noexcept { x += r.x; y += r.y; z += r.z; return *this; }
    Vec3& operator-=(const Vec3& r) noexcept { x -= r.x; y -= r.y; z -= r.z; return *this; }
    Vec3& operator*=(float s) noexcept { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) noexcept { x /= s; y /= s; z /= s; return *this; }

    constexpr float LengthSq() const noexcept { return x * x + y * y + z * z; }
    float Length() const noexcept { return std::sqrt(LengthSq()); }

    static constexpr float Dot(const Vec3& a, const Vec3& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static constexpr Vec3 Cross(const Vec3& a, const Vec3& b) noexcept
    {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    static inline Vec3 Normalize(const Vec3& v, float eps = 1e-6f) noexcept
    {
        const float lenSq = v.LengthSq();
        if (lenSq <= eps * eps) return Vec3(0.0f);
        const float inv = 1.0f / std::sqrt(lenSq);
        return Vec3(v.x * inv, v.y * inv, v.z * inv);
    }
};

inline constexpr Vec3 operator*(float s, const Vec3& v) noexcept
{
    return Vec3(v.x * s, v.y * s, v.z * s);
}