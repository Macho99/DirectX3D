#include "pch.h"
#include "AABBBoxCollider.h"
#include "OBBBoxCollider.h"
#include "SphereCollider.h"

AABBBoxCollider::AABBBoxCollider() 
	:Super(ColliderType::AABB)
{
}

AABBBoxCollider::~AABBBoxCollider()
{
}

void AABBBoxCollider::Update()
{
}

bool AABBBoxCollider::Intersects(Ray& ray, OUT float& distance)
{
	return _boundingBox.Intersects(ray.position, ray.direction, OUT distance);
}

bool AABBBoxCollider::Intersects(shared_ptr<BaseCollider>& other)
{
    ColliderType otherType = other->GetColliderType();

    switch (otherType)
    {
    case ColliderType::Sphere:
        return _boundingBox.Intersects(dynamic_pointer_cast<SphereCollider>(other)->GetBoundingSphere());
    case ColliderType::AABB:
        return _boundingBox.Intersects(dynamic_pointer_cast<AABBBoxCollider>(other)->GetBoundingBox());
    case ColliderType::OBB:
        return _boundingBox.Intersects(dynamic_pointer_cast<OBBBoxCollider>(other)->GetBoundingBox());
    }

    return false;
}
