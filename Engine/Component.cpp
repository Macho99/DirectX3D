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

void Component::InvokeStart()
{
	if (_isStarted)
		return;

	_isStarted = true;
	Start();
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

bool Component::IsActiveAndEnabled() const
{
	GameObject* gameObject = _gameObject.Resolve();
	return _enabled && gameObject != nullptr && gameObject->IsActiveInHierarchy();
}

void Component::SetEnabled(bool enabled)
{
	if (_enabled == enabled)
		return;

	GameObject* gameObject = _gameObject.Resolve();
	bool notifyLifecycle = gameObject != nullptr && gameObject->IsActiveInHierarchy();
	_enabled = enabled;

	if (notifyLifecycle)
	{
		if (_enabled)
			OnEnable();
		else
			OnDisable();
	}
}

bool Component::OnGUI()
{
	return false;
}

void Component::OnMenu()
{
}
