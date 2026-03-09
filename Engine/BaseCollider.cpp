#include "pch.h"
#include "BaseCollider.h"

BaseCollider::BaseCollider(ComponentType componentType, ColliderType colliderType)
	:Super(componentType), _colliderType(colliderType)
{
}

BaseCollider::~BaseCollider()
{
}
