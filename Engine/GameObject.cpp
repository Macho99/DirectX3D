#include "pch.h"
#include "GameObject.h"
#include "MonoBehaviour.h"
#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include "BaseCollider.h"
#include "MeshRenderer.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "Terrain.h"
#include "Button.h"
#include "Billboard.h"
#include "SnowBillboard.h"
#include "Renderer.h"
#include "Scene.h"
#include "SlotManager.h"

GameObject::GameObject(string name) : _name(name), _components{}
{

}

GameObject::~GameObject()
{

}

void GameObject::Awake()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->Awake();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->Awake();
	}
}

void GameObject::Start()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->Start();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->Start();
	}
}

void GameObject::Update()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->Update();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->Update();
	}
}

void GameObject::LateUpdate()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->LateUpdate();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->LateUpdate();
	}
}

void GameObject::FixedUpdate()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->FixedUpdate();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->FixedUpdate();
	}
}

void GameObject::OnDestroy()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->OnDestroy();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->OnDestroy();
	}
}

void GameObject::OnInspectorFocus()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->OnInspectorFocus();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->OnInspectorFocus();
	}
}

void GameObject::OnInspectorFocusLost()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid())
			component.Resolve()->OnInspectorFocusLost();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		script.Resolve()->OnInspectorFocusLost();
	}
}

Component* GameObject::GetFixedComponent(ComponentType type)
{
	uint8 index = static_cast<uint8>(type);
	assert(index < FIXED_COMPONENT_COUNT);
	return _components[index].Resolve();
}

Transform* GameObject::GetTransform()
{
	Component* component = GetFixedComponent(ComponentType::Transform);
	if (component == nullptr)
	{
		AddComponent(make_unique<Transform>());
		component = GetFixedComponent(ComponentType::Transform);
	}
	return static_cast<Transform*>(component);
}

Camera* GameObject::GetCamera()
{
	Component* component = GetFixedComponent(ComponentType::Camera);
	return static_cast<Camera*>(component);
}

Light* GameObject::GetLight()
{
	Component* component = GetFixedComponent(ComponentType::Light);
	return static_cast<Light*>(component);
}

MeshRenderer* GameObject::GetMeshRenderer()
{
	Component* component = GetFixedComponent(ComponentType::MeshRenderer);
	return static_cast<MeshRenderer*>(component);
}

ModelRenderer* GameObject::GetModelRenderer()
{
	Component* component = GetFixedComponent(ComponentType::ModelRenderer);
	return static_cast<ModelRenderer*>(component);
}

ModelAnimator* GameObject::GetModelAnimator()
{
	Component* component = GetFixedComponent(ComponentType::ModelAnimator);
	return static_cast<ModelAnimator*>(component);
}

Renderer* GameObject::GetRenderer()
{
	Component* renderer = GetFixedComponent(ComponentType::MeshRenderer);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::ModelRenderer);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::ModelAnimator);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::ParticleSystem);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::Billboard);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::SnowBillboard);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::TessTerrain);
    if (renderer == nullptr)
        renderer = GetFixedComponent(ComponentType::GrassRenderer);

	return static_cast<Renderer*>(renderer);
}

BaseCollider* GameObject::GetCollider()
{
	Component* component = GetFixedComponent(ComponentType::Collider);
	return static_cast<BaseCollider*>(component);
}

Terrain* GameObject::GetTerrain()
{
	Component* component = GetFixedComponent(ComponentType::Terrain);
	return static_cast<Terrain*>(component);
}

Button* GameObject::GetButton()
{
	Component* component = GetFixedComponent(ComponentType::Button);
	return static_cast<Button*>(component);
}

Billboard* GameObject::GetBillboard()
{
	Component* component = GetFixedComponent(ComponentType::Billboard);
	return static_cast<Billboard*>(component);
}

SnowBillboard* GameObject::GetSnowBillboard()
{
	Component* component = GetFixedComponent(ComponentType::SnowBillboard);
	return static_cast<SnowBillboard*>(component);
}

//std::shared_ptr<Animator> GameObject::GetAnimator()
//{
//	shared_ptr<Component> component = GetFixedComponent(ComponentType::Animator);
//	return static_pointer_cast<Animator>(component);
//}

void GameObject::AddComponent(unique_ptr<Component> component)
{
    Guid::SetCurrentInstanceId(CUR_SCENE->GetInstanceId());
	GameObjectRef thisRef(_guid);
	component->SetGameObject(thisRef);
	uint8 index = static_cast<uint8>(component->GetType());
	GuidRef guidRef = CUR_SCENE->AddComponent(thisRef, std::move(component));
	if (index < FIXED_COMPONENT_COUNT)
	{
		ComponentRefBase componentRefBase(guidRef);
		_components[index] = componentRefBase;
        _components[index].Resolve();
	}
	else
	{
        ComponentRef<MonoBehaviour> scriptRef(guidRef);
		_scripts.push_back((scriptRef));
        _scripts.back().Resolve();
	}
}

void GameObject::SetActive(bool active)
{
    if (_localActive == active)
        return;

	_localActive = active;
	bool parentActive = true;
	Transform* parentTransform;
	if (GetTransform()->TryGetParent(OUT parentTransform))
	{
		parentActive = parentTransform->GetGameObject()->IsActive();
	}
	UpdateActiveInHierarchy(parentActive, false);
}

void GameObject::UpdateActiveInHierarchy(bool parentActive, bool forceUpdate)
{
	bool newActive = parentActive && _localActive;
    if (forceUpdate == false && _isActive == newActive)
        return;

    _isActive = newActive;
    //for (ComponentRefBase& component : _components)
    //{
    //    if (component.IsValid())
    //        component.Resolve()->SetActive(_isActive);
    //}
	//
    //for (ComponentRef<MonoBehaviour>& script : _scripts)
    //{
    //    if (script.IsValid())
    //        script.Resolve()->SetActive(_isActive);
    //}

    vector<TransformRef>& children = GetTransform()->GetChildren();
    for (TransformRef& child : children)
    {
        Transform* childTransform = child.Resolve();
        ASSERT(childTransform != nullptr);
		GameObject* childGameObject = childTransform->GetGameObject();
		ASSERT(childGameObject != nullptr);
		childGameObject->UpdateActiveInHierarchy(_isActive, forceUpdate);
    }
}

GameObjectRef GameObject::GetGameObjectRefByGuid(const Guid& guid)
{
    return GameObjectRef(guid);
}
