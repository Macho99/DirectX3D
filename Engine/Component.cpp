#include "pch.h"
#include "Component.h"
#include "GameObject.h"

Component::Component(ComponentType type) : _type(type)
{

}

Component::~Component()
{

}

GameObject* Component::GetGameObject()
{
	return _gameObject.Resolve();
}

Transform* Component::GetTransform()
{
	return _gameObject.Resolve()->GetTransform();
}
