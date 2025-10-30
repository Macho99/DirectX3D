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

GameObject::GameObject()
{
}

GameObject::~GameObject()
{

}

void GameObject::Awake()
{
	for (shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Awake();
	}

	for (shared_ptr<MonoBehaviour>& script : _scripts)
	{
		script->Awake();
	}
}

void GameObject::Start()
{
	for (shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Start();
	}

	for (shared_ptr<MonoBehaviour>& script : _scripts)
	{
		script->Start();
	}
}

void GameObject::Update()
{
	for (shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Update();
	}

	for (shared_ptr<MonoBehaviour>& script : _scripts)
	{
		script->Update();
	}
}

void GameObject::LateUpdate()
{
	for (shared_ptr<Component>& component : _components)
	{
		if (component)
			component->LateUpdate();
	}

	for (shared_ptr<MonoBehaviour>& script : _scripts)
	{
		script->LateUpdate();
	}
}

void GameObject::FixedUpdate()
{
	for (shared_ptr<Component>& component : _components)
	{
		if (component)
			component->FixedUpdate();
	}

	for (shared_ptr<MonoBehaviour>& script : _scripts)
	{
		script->FixedUpdate();
	}
}

std::shared_ptr<Component> GameObject::GetFixedComponent(ComponentType type)
{
	uint8 index = static_cast<uint8>(type);
	assert(index < FIXED_COMPONENT_COUNT);
	return _components[index];
}

std::shared_ptr<Transform> GameObject::GetTransform()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Transform);
	if (component == nullptr)
	{
		component = make_shared<Transform>();
		AddComponent(component);
	}
	return static_pointer_cast<Transform>(component);
}

std::shared_ptr<Camera> GameObject::GetCamera()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Camera);
	return static_pointer_cast<Camera>(component);
}

shared_ptr<Light> GameObject::GetLight()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Light);
	return static_pointer_cast<Light>(component);
}

shared_ptr<MeshRenderer> GameObject::GetMeshRenderer()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::MeshRenderer);
	return static_pointer_cast<MeshRenderer>(component);
}

shared_ptr<ModelRenderer> GameObject::GetModelRenderer()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::ModelRenderer);
	return static_pointer_cast<ModelRenderer>(component);
}

shared_ptr<ModelAnimator> GameObject::GetModelAnimator()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Animator);
	return static_pointer_cast<ModelAnimator>(component);
}

shared_ptr<Renderer> GameObject::GetRenderer()
{
	shared_ptr<Component> renderer = GetFixedComponent(ComponentType::MeshRenderer);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::ModelRenderer);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::Animator);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::ParticleSystem);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::Billboard);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::SnowBillboard);
	if (renderer == nullptr)
		renderer = GetFixedComponent(ComponentType::TessTerrain);

	return static_pointer_cast<Renderer>(renderer);
}

shared_ptr<BaseCollider> GameObject::GetCollider()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Collider);
	return static_pointer_cast<BaseCollider>(component);
}

shared_ptr<Terrain> GameObject::GetTerrain()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Terrain);
	return static_pointer_cast<Terrain>(component);
}

shared_ptr<Button> GameObject::GetButton()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Button);
	return static_pointer_cast<Button>(component);
}

shared_ptr<Billboard> GameObject::GetBillboard()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::Billboard);
	return static_pointer_cast<Billboard>(component);
}

shared_ptr<SnowBillboard> GameObject::GetSnowBillboard()
{
	shared_ptr<Component> component = GetFixedComponent(ComponentType::SnowBillboard);
	return static_pointer_cast<SnowBillboard>(component);
}

//std::shared_ptr<Animator> GameObject::GetAnimator()
//{
//	shared_ptr<Component> component = GetFixedComponent(ComponentType::Animator);
//	return static_pointer_cast<Animator>(component);
//}

void GameObject::AddComponent(shared_ptr<Component> component)
{
	component->SetGameObject(shared_from_this());

	uint8 index = static_cast<uint8>(component->GetType());
	if (index < FIXED_COMPONENT_COUNT)
	{
		_components[index] = component;
	}
	else
	{
		_scripts.push_back(dynamic_pointer_cast<MonoBehaviour>(component));
	}
}