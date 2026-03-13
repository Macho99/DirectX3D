#include "pch.h"
#include "AABBBoxCollider.h"
#include "OBBBoxCollider.h"
#include "SphereCollider.h"
#include "OnGUIUtils.h"

AABBBoxCollider::AABBBoxCollider() 
	:Super(StaticType, ColliderType::AABB)
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

bool AABBBoxCollider::Intersects(BaseCollider* other)
{
    ColliderType otherType = other->GetColliderType();

    switch (otherType)
    {
    case ColliderType::Sphere:
        return _boundingBox.Intersects(static_cast<SphereCollider*>(other)->GetBoundingSphere());
    case ColliderType::AABB:
        return _boundingBox.Intersects(static_cast<AABBBoxCollider*>(other)->GetBoundingBox());
    case ColliderType::OBB:
        return _boundingBox.Intersects(static_cast<OBBBoxCollider*>(other)->GetBoundingBox());
    }

    return false;
}

bool AABBBoxCollider::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    changed |= OnGUIUtils::DrawFloat3("Center", &_boundingBox.Center.x, 0.1f);
    changed |= OnGUIUtils::DrawFloat3("Extents", &_boundingBox.Extents.x, 0.1f);
    return changed;
}
