#include "pch.h"
#include "OBBBoxCollider.h"
#include "SphereCollider.h"
#include "AABBBoxCollider.h"

OBBBoxCollider::OBBBoxCollider() : BaseCollider(StaticType, ColliderType::OBB)
{

}

OBBBoxCollider::~OBBBoxCollider()
{

}

void OBBBoxCollider::Update()
{
    _boundingBox.Center = GetTransform()->GetPosition();
	_boundingBox.Extents = GetTransform()->GetScale() * 0.5f;
}

bool OBBBoxCollider::Intersects(Ray& ray, OUT float& distance)
{
	return _boundingBox.Intersects(ray.position, ray.direction, OUT distance);
}

bool OBBBoxCollider::Intersects(BaseCollider* other)
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

bool OBBBoxCollider::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();
    changed |= ImGui::DragFloat3("Center", &_boundingBox.Center.x, 0.1f);
    changed |= ImGui::DragFloat3("Extents", &_boundingBox.Extents.x, 0.1f);
    changed |= ImGui::DragFloat4("Orientation", &_boundingBox.Orientation.x, 0.1f);
    return changed;
}
