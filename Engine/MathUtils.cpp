#include "pch.h"
#include "MathUtils.h"

bool MathUtils::PointInSphere(const Point3D& point, const Sphere3D sphere)
{
	float magSq = (point - sphere.position).LengthSquared();
	float radSq = sphere.radius * sphere.radius;
	return magSq <= radSq;
}

Point3D MathUtils::ClosestPoint(const Sphere3D& sphere, const Point3D& point)
{
	Vec3 sphereToPointDir = (point - sphere.position);
	sphereToPointDir.Normalize();
	return sphere.position + sphereToPointDir * sphere.radius;
}

bool MathUtils::PointInAABB(const Point3D& point, const AABB3D& aabb)
{
	Point3D min = AABB3D::GetMin(aabb);
	Point3D max = AABB3D::GetMax(aabb);

	if (point.x < min.x || point.y < min.y || point.z < min.z)
		return false;
	if (point.x > max.x || point.y > max.y || point.z > max.z)
		return false;

	return true;
}


Point3D MathUtils::ClosestPoint(const AABB3D& aabb, const Point3D& point)
{
	Point3D result = point;
	Point3D minPt = AABB3D::GetMin(aabb);
	Point3D maxPt = AABB3D::GetMax(aabb);

	result.x = max(result.x, minPt.x);
	result.y = max(result.y, minPt.y);
	result.z = max(result.z, minPt.z);

	result.x = min(result.x, maxPt.x);
	result.y = min(result.y, maxPt.y);
	result.z = min(result.z, maxPt.z);

	return result;
}


Point3D MathUtils::ClosestPoint(const OBB3D& obb, const Point3D& point)
{
	Point3D result = obb.position;
	Vec3 dir = point - obb.position;

	vector<Vec3> axis;
	axis.push_back(obb.orientation.Right());
	axis.push_back(obb.orientation.Up());
	axis.push_back(obb.orientation.Backward());

	vector<float> size;
	size.push_back(obb.size.x);
	size.push_back(obb.size.y);
	size.push_back(obb.size.z);

	for (int i = 0; i < 3; ++i)
	{
		float distance = dir.Dot(axis[i]);

		if (distance > size[i])
			distance = size[i];

		if (distance < -size[i])
			distance = -size[i];

		result = result + (axis[i] * distance);
	}

	return result;
}

bool MathUtils::PointInOBB(const Point3D& point, const OBB3D& obb)
{
	Vec3 dir = point - obb.position;

	vector<Vec3> axis;
	axis.push_back(obb.orientation.Right());
	axis.push_back(obb.orientation.Up());
	axis.push_back(obb.orientation.Backward());

	vector<float> size;
	size.push_back(obb.size.x);
	size.push_back(obb.size.y);
	size.push_back(obb.size.z);

	for (int32 i = 0; i < 3; i++)
	{
		float distance = dir.Dot(axis[i]);

		if (distance > size[i])
			return false;
		if (distance < -size[i])
			return false;
	}

	return true;
}

bool MathUtils::PointOnPlane(const Point3D& point, const Plane3D& plane)
{
	float dot = point.Dot(plane.normal);
	// CMP(dot - plane.distance, 0.0f);
	return dot == plane.distance;
}

Point3D MathUtils::ClosestPoint(const Plane3D& plane, const Point3D& point)
{
	float dot = point.Dot(plane.normal);
	float distance = dot - plane.distance;
	return point - plane.normal * distance;
}


Point3D MathUtils::ClosestPoint(const Line3D& line, const Point3D& point)
{
	Vec3 lVec = line.end - line.start; // Line Vector
	float t = (point - line.start).Dot(lVec) / lVec.Dot(lVec);
	t = fmaxf(t, 0.0f); // Clamp to 0
	t = fminf(t, 1.0f); // Clamp to 1
	return line.start + lVec * t;
}

bool MathUtils::PointOnLine(const Point3D& point, const Line3D& line)
{
	Point3D closest = ClosestPoint(line, point);
	float distanceSq = (closest - point).LengthSquared();
	// CMP(distanceSq, 0.0f);
	return distanceSq == 0.f;
}


bool MathUtils::PointOnRay(const Point3D& point, const Ray3D& ray)
{
	if (point == ray.origin)
		return true;

	Vec3 norm = point - ray.origin;
	norm.Normalize();

	float diff = norm.Dot(ray.direction);
	return diff == 1.0f; // Consider using epsilon!
}

Point3D MathUtils::ClosestPoint(const Ray3D& ray, const Point3D& point)
{
	float t = (point - ray.origin).Dot(ray.direction);

	t = fmaxf(t, 0.0f);

	return Point3D(ray.origin + ray.direction * t);
}


bool MathUtils::SphereSphere(const Sphere3D& s1, const Sphere3D& s2)
{
	float sum = s1.radius + s2.radius;
	float sqDistance = (s1.position - s2.position).LengthSquared();
	return sqDistance <= sum * sum;
}

bool MathUtils::SphereAABB(const Sphere3D& sphere, const AABB3D& aabb)
{
	Point3D closestPoint = ClosestPoint(aabb, sphere.position);
	float distSq = (sphere.position - closestPoint).LengthSquared();
	float radiusSq = sphere.radius * sphere.radius;
	return distSq < radiusSq;
}

bool MathUtils::SphereOBB(const Sphere3D& sphere, const OBB3D& obb)
{
	Point3D closestPoint = ClosestPoint(obb, sphere.position);
	float distSq = (sphere.position - closestPoint).LengthSquared();
	float radiusSq = sphere.radius * sphere.radius;
	return distSq < radiusSq;
}

bool MathUtils::SpherePlane(const Sphere3D& sphere, const Plane3D& plane)
{
	Point3D closestPoint = ClosestPoint(plane, sphere.position);
	float distSq = (sphere.position - closestPoint).LengthSquared();
	float radiusSq = sphere.radius * sphere.radius;
	return distSq < radiusSq;
}


bool MathUtils::AABBAABB(const AABB3D& aabb1, const AABB3D& aabb2)
{
	Point3D aMin = AABB3D::GetMin(aabb1);
	Point3D aMax = AABB3D::GetMax(aabb1);
	Point3D bMin = AABB3D::GetMin(aabb2);
	Point3D bMax = AABB3D::GetMax(aabb2);

	return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
		(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
		(aMin.z <= bMax.z && aMax.z >= bMin.z);
}


Interval3D MathUtils::GetInterval(const AABB3D& aabb, const Vec3& axis)
{
	Vec3 i = AABB3D::GetMin(aabb);
	Vec3 a = AABB3D::GetMax(aabb);

	Vec3 vertex[8] =
	{
		Vec3(i.x, a.y, a.z),
		Vec3(i.x, a.y, i.z),
		Vec3(i.x, i.y, a.z),
		Vec3(i.x, i.y, i.z),
		Vec3(a.x, a.y, a.z),
		Vec3(a.x, a.y, i.z),
		Vec3(a.x, i.y, a.z),
		Vec3(a.x, i.y, i.z)
	};

	// 최소/최대 구하기
	Interval3D result;
	result.min = result.max = axis.Dot(vertex[0]);

	for (int i = 1; i < 8; ++i)
	{
		float projection = axis.Dot(vertex[i]);
		result.min = min(result.min, projection);
		result.max = max(result.max, projection);
	}

	return result;
}


Interval3D MathUtils::GetInterval(const OBB3D& obb, const Vec3& axis)
{
	Vec3 vertex[8];

	Vec3 C = obb.position; // OBB Center
	Vec3 E = obb.size; // OBB Extents

	vector<Vec3> A; // OBB Axis
	A.push_back(obb.orientation.Right());
	A.push_back(obb.orientation.Up());
	A.push_back(obb.orientation.Backward());

	// 각 좌표를 구해준다
	vertex[0] = C + A[0] * E.x + A[1] * E.y + A[2] * E.z;
	vertex[1] = C - A[0] * E.x + A[1] * E.y + A[2] * E.z;
	vertex[2] = C + A[0] * E.x - A[1] * E.y + A[2] * E.z;
	vertex[3] = C + A[0] * E.x + A[1] * E.y - A[2] * E.z;
	vertex[4] = C - A[0] * E.x - A[1] * E.y - A[2] * E.z;
	vertex[5] = C + A[0] * E.x - A[1] * E.y - A[2] * E.z;
	vertex[6] = C - A[0] * E.x + A[1] * E.y - A[2] * E.z;
	vertex[7] = C - A[0] * E.x - A[1] * E.y + A[2] * E.z;

	// 최소/최대 구하기
	Interval3D result;
	result.min = result.max = axis.Dot(vertex[0]);

	for (int i = 1; i < 8; ++i)
	{
		float projection = axis.Dot(vertex[i]);
		result.min = min(result.min, projection);
		result.max = max(result.max, projection);
	}

	return result;
}

bool MathUtils::OverlapOnAxis(const AABB3D& aabb, const OBB3D& obb, const Vec3& axis)
{
	Interval3D a = GetInterval(aabb, axis);
	Interval3D b = GetInterval(obb, axis);
	return ((b.min <= a.max) && (a.min <= b.max));
}

bool MathUtils::AABBOBB(const AABB3D& aabb, const OBB3D& obb)
{
	Vec3 test[15] =
	{
		Vec3(1, 0, 0),				// AABB axis 1
		Vec3(0, 1, 0),				// AABB axis 2
		Vec3(0, 0, 1),				// AABB axis 3
		obb.orientation.Right(),	// OBB axis 1
		obb.orientation.Up(),		// OBB axis 2
		obb.orientation.Backward()  // OBB axis 3
		// We will fill out the remaining axis in the next step
	};

	for (int i = 0; i < 3; ++i)
	{
		// Fill out rest of axis
		test[6 + i * 3 + 0] = test[i].Cross(test[0]);
		test[6 + i * 3 + 1] = test[i].Cross(test[1]);
		test[6 + i * 3 + 2] = test[i].Cross(test[2]);
	}

	for (int i = 0; i < 15; ++i)
	{
		if (OverlapOnAxis(aabb, obb, test[i]) == false)
			return false; // Seperating axis found
	}

	return true; // Seperating axis not found
}

bool MathUtils::AABBPlane(const AABB3D& aabb, const Plane3D& plane)
{
	float pLen = aabb.size.x * fabsf(plane.normal.x) +
		aabb.size.y * fabsf(plane.normal.y) +
		aabb.size.z * fabsf(plane.normal.z);

	float dot = plane.normal.Dot(aabb.position);
	float dist = dot - plane.distance;

	return fabsf(dist) <= pLen;
}

bool MathUtils::OverlapOnAxis(const OBB3D& obb1, const OBB3D& obb2, const Vec3& axis)
{
	Interval3D a = GetInterval(obb1, axis);
	Interval3D b = GetInterval(obb1, axis);
	return ((b.min <= a.max) && (a.min <= b.max));
}

bool MathUtils::OBBOBB(const OBB3D& obb1, const OBB3D& obb2)
{
	Vec3 test[15] =
	{
		obb1.orientation.Right(),
		obb1.orientation.Up(),
		obb1.orientation.Backward(),
		obb2.orientation.Right(),
		obb2.orientation.Up(),
		obb2.orientation.Backward(),
	};

	for (int i = 0; i < 3; ++i)
	{
		// Fill out rest of axis
		test[6 + i * 3 + 0] = test[i].Cross(test[0]);
		test[6 + i * 3 + 1] = test[i].Cross(test[1]);
		test[6 + i * 3 + 2] = test[i].Cross(test[2]);
	}

	for (int i = 0; i < 15; ++i)
	{
		if (OverlapOnAxis(obb1, obb2, test[i]) == false)
			return false; // Seperating axis found
	}

	return true; // Seperating axis not found
}

bool MathUtils::PlanePlane(const Plane3D& plane1, const Plane3D& plane2)
{
	Vec3 d = plane1.normal.Cross(plane2.normal);
	return d.Dot(d) != 0; // Consider using an epsilon!
}



bool MathUtils::Raycast(const Sphere3D& sphere, const Ray3D& ray, OUT float& distance)
{
	Vec3 e = sphere.position - ray.origin;

	float rSq = sphere.radius * sphere.radius;
	float eSq = e.LengthSquared();

	float a = e.Dot(ray.direction);

	float bSq = eSq - (a * a);
	float f = sqrt(rSq - bSq);

	// No collision has happened
	if (rSq - (eSq - (a * a)) < 0.0f)
		return false;

	// Ray starts inside the sphere
	if (eSq < rSq)
	{
		distance = a + f;
		return true;
	}

	// else Normal intersection
	distance = a - f;
	return true;
}


// Cyrus-Beck clipping
// AABB를 구성하는 6개의 평면에 대해 클리핑 진행
// Point3D point = ray.origin + ray.direction * t;
bool MathUtils::Raycast(const AABB3D& aabb, const Ray3D& ray, OUT float& distance)
{
	Vec3 min = AABB3D::GetMin(aabb);
	Vec3 max = AABB3D::GetMax(aabb);

	// TODO : 0 나누기 방지 위해 +@ 더해줌
	float t1 = (min.x - ray.origin.x) / ray.direction.x;
	float t2 = (max.x - ray.origin.x) / ray.direction.x;

	float t3 = (min.y - ray.origin.y) / ray.direction.y;
	float t4 = (max.y - ray.origin.y) / ray.direction.y;

	float t5 = (min.z - ray.origin.z) / ray.direction.z;
	float t6 = (max.z - ray.origin.z) / ray.direction.z;

	// Largest min value
	float tmin = fmaxf(
		fmaxf(
			fminf(t1, t2),
			fminf(t3, t4)
		),
		fminf(t5, t6)
	);

	// Smallest max value
	float tmax = fminf(
		fminf(
			fmaxf(t1, t2),
			fmaxf(t3, t4)
		),
		fmaxf(t5, t6)
	);

	if (tmax < 0)
		return false;

	if (tmin > tmax)
		return false;

	if (tmin < 0.0f)
	{
		distance = tmax;
		return true;
	}

	distance = tmin;
	return true;
}


bool MathUtils::Raycast(const Plane3D& plane, const Ray3D& ray, OUT float& distance)
{
	float nd = ray.direction.Dot(plane.normal);
	float pn = ray.origin.Dot(plane.normal);

	if (nd >= 0.0f)
		return false;

	float t = (plane.distance - pn) / nd;

	if (t >= 0.0f)
	{
		distance = t;
		return true;
	}

	return false;
}


//-------------------
// Triangle
//-------------------

bool MathUtils::PointInTriangle(const Point3D& p, const Triangle3D& t)
{
	Vec3 a = t.a - p;
	Vec3 b = t.b - p;
	Vec3 c = t.c - p;

	Vec3 normPBC = b.Cross(c); // Normal of PBC (u)
	Vec3 normPCA = c.Cross(a); // Normal of PCA (v)
	Vec3 normPAB = a.Cross(b); // Normal of PAB (w)

	if (normPBC.Dot(normPCA) < 0.0f)
		return false;

	else if (normPBC.Dot(normPAB) < 0.0f)
		return false;

	return true;
}


Vec3 MathUtils::Barycentric(const Point3D& p, const Triangle3D& t)
{
	Vec3 ap = p - t.a;
	Vec3 bp = p - t.b;
	Vec3 cp = p - t.c;

	Vec3 ab = t.b - t.a;
	Vec3 ac = t.c - t.a;
	Vec3 bc = t.c - t.b;
	Vec3 cb = t.b - t.c;
	Vec3 ca = t.a - t.c;

	Vec3 v = ab - ProjectVecOnVec(ab, cb);
	float a = 1.0f - (v.Dot(ap) / v.Dot(ab));

	v = bc - ProjectVecOnVec(bc, ac);
	float b = 1.0f - (v.Dot(bp) / v.Dot(bc));

	v = ca - ProjectVecOnVec(ca, ab);
	float c = 1.0f - (v.Dot(cp) / v.Dot(ca));

	return Vec3(a, b, c);
}

Plane3D MathUtils::FromTriangle(const Triangle3D& t)
{
	Plane3D result;

	result.normal = (t.b - t.a).Cross(t.c - t.a);
	result.normal.Normalize();

	result.distance = result.normal.Dot(t.a);

	return result;
}

bool MathUtils::Raycast(const Triangle3D& triangle, const Ray3D& ray, float& distance)
{
	Plane3D plane = FromTriangle(triangle);

	float t = 0;
	if (Raycast(plane, ray, OUT t) == false)
		return false;

	Point3D result = ray.origin + ray.direction * t;

	Vec3 barycentric = Barycentric(result, triangle);

	if (barycentric.x >= 0.0f && barycentric.x <= 1.0f &&
		barycentric.y >= 0.0f && barycentric.y <= 1.0f &&
		barycentric.z >= 0.0f && barycentric.z <= 1.0f)
	{
		distance = t;
		return true;
	}

	return false;
}


Vec3 MathUtils::ProjectVecOnVec(Vec3 a, Vec3 b)
{
	b.Normalize();

	float dist = a.Dot(b);

	return b * dist;
}
