#include "pch.h"
#include "Component.h"
#include "GameObject.h"
#include "Transform.h"

Component::Component() : _type(ComponentType::End)
{
}

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

TransformRef Component::GetTransformRef()
{
    return _gameObject.Resolve()->GetTransformRef();
}

bool Component::OnGUI()
{
	return false;
}

void Component::OnMenu()
{
}