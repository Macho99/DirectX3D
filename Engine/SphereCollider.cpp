#include "pch.h"
#include "SphereCollider.h"
#include "AABBBoxCollider.h"
#include "OBBBoxCollider.h"

SphereCollider::SphereCollider()
    :Super(StaticType, ColliderType::Sphere)
{
}

SphereCollider::~SphereCollider()
{
}

void SphereCollider::Update()
{
    _boundingSphere.Center = GetGameObject()->GetTransform()->GetPosition();

    Vec3 scale = GetGameObject()->GetTransform()->GetScale();
    _boundingSphere.Radius = _radius * max(max(scale.x, scale.y), scale.z);
}

bool SphereCollider::Intersects(Ray& ray, OUT float& distance)
{
    return _boundingSphere.Intersects(ray.position, ray.direction, OUT distance);
}

bool SphereCollider::Intersects(BaseCollider* other)
{
    ColliderType otherType = other->GetColliderType();

    switch (otherType)
    {
        case ColliderType::Sphere:
            return _boundingSphere.Intersects(static_cast<SphereCollider*>(other)->GetBoundingSphere());
        case ColliderType::AABB:
            return _boundingSphere.Intersects(static_cast<AABBBoxCollider*>(other)->GetBoundingBox());
        case ColliderType::OBB:
            return _boundingSphere.Intersects(static_cast<OBBBoxCollider*>(other)->GetBoundingBox());
    }

    return false;
}

bool SphereCollider::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    changed |= ImGui::DragFloat("Radius", &_radius, 0.1f);
    changed |= ImGui::DragFloat3("Center", &_boundingSphere.Center.x, 0.1f);
    return changed;
}
