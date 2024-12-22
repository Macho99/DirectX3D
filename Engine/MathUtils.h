#pragma once
#include "Primitive3D.h"


struct MathUtils
{
	//-------------------
	// Point Test
	//-------------------

	// Sphere2Point
	static bool PointInSphere(const Point3D& point, const Sphere3D sphere);
	static Point3D ClosestPoint(const Sphere3D& sphere, const Point3D& point);

	// AABB2Point
	static bool PointInAABB(const Point3D& point, const AABB3D& aabb);
	static Point3D ClosestPoint(const AABB3D& aabb, const Point3D& point);

	// OBB2Point
	static bool PointInOBB(const Point3D& point, const OBB3D& obb);
	static Point3D ClosestPoint(const OBB3D& obb, const Point3D& point);

	// Plane2Point
	static bool PointOnPlane(const Point3D& point, const Plane3D& plane);
	static Point3D ClosestPoint(const Plane3D& plane, const Point3D& point);

	// Line2Point
	static bool PointOnLine(const Point3D& point, const Line3D& line);
	static Point3D ClosestPoint(const Line3D& line, const Point3D& point);

	// Ray2Point
	static bool PointOnRay(const Point3D& point, const Ray3D& ray);
	static Point3D ClosestPoint(const Ray3D& ray, const Point3D& point);

	//-------------------
	// Intersection
	//-------------------

	static bool SphereSphere(const Sphere3D& s1, const Sphere3D& s2);
	static bool SphereAABB(const Sphere3D& sphere, const AABB3D& aabb);
	static bool SphereOBB(const Sphere3D& sphere, const OBB3D& obb);
	static bool SpherePlane(const Sphere3D& sphere, const Plane3D& plane);
	static bool AABBAABB(const AABB3D& aabb1, const AABB3D& aabb2);

	static Interval3D GetInterval(const AABB3D& aabb, const Vec3& axis);
	static Interval3D GetInterval(const OBB3D& obb, const Vec3& axis);
	static bool OverlapOnAxis(const AABB3D& aabb, const OBB3D& obb, const Vec3& axis);
	static bool AABBOBB(const AABB3D& aabb, const OBB3D& obb);

	static bool OverlapOnAxis(const OBB3D& obb1, const OBB3D& obb2, const Vec3& axis);
	static bool AABBPlane(const AABB3D& aabb, const Plane3D& plane);
	static bool OBBOBB(const OBB3D& obb1, const OBB3D& obb2);
	static bool PlanePlane(const Plane3D& plane1, const Plane3D& plane2);

	//-------------------
	// Raycast
	//-------------------
	static bool Raycast(const Sphere3D& sphere, const Ray3D& ray, OUT float& distance);
	static bool Raycast(const AABB3D& aabb, const Ray3D& ray, OUT float& distance);
	static bool Raycast(const Plane3D& plane, const Ray3D& ray, OUT float& distance);

	static bool PointInTriangle(const Point3D& p, const Triangle3D& t);
	
	static Plane3D FromTriangle(const Triangle3D& t);
	static Vec3 Barycentric(const Point3D& p, const Triangle3D& t);
	static bool Raycast(const Triangle3D& triangle, const Ray3D& ray, float& distance);
	static Vec3 ProjectVecOnVec(Vec3 from, Vec3 to);
};

