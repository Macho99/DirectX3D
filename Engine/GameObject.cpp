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
#include "LineRenderer.h"
#include "TrailRenderer.h"
#include "Terrain.h"
#include "Button.h"
#include "UIImage.h"
#include "Billboard.h"
#include "SnowBillboard.h"
#include "Renderer.h"
#include "Scene.h"
#include "SlotManager.h"
#include "OnGUIUtils.h"

vector<ComponentType> GameObject::S_RendererTypes = {
	ComponentType::InputText,
	ComponentType::Text,
	ComponentType::Button,
	ComponentType::UIImage,
	ComponentType::ScrollView,
	ComponentType::MeshRenderer,
	ComponentType::ModelRenderer,
	ComponentType::ModelAnimator,
	ComponentType::ParticleSystem,
	ComponentType::Billboard,
	ComponentType::SnowBillboard,
	ComponentType::TessTerrain,
	ComponentType::GrassRenderer,
	ComponentType::LineRenderer,
	ComponentType::TrailRenderer,
	ComponentType::SsrRenderer
};

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

void GameObject::Update()
{
	for (ComponentRefBase& component : _components)
	{
		Component* componentPtr = component.Resolve();
		if (componentPtr != nullptr && componentPtr->IsEnabled())
		{
			componentPtr->InvokeStart();
			componentPtr->Update();
		}
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		MonoBehaviour* scriptPtr = script.Resolve();
		if (scriptPtr != nullptr && scriptPtr->IsEnabled())
		{
			scriptPtr->InvokeStart();
			scriptPtr->Update();
		}
	}
}

void GameObject::LateUpdate()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid() && component.Resolve()->IsEnabled())
			component.Resolve()->LateUpdate();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		if (script.Resolve()->IsEnabled())
			script.Resolve()->LateUpdate();
	}
}

void GameObject::FixedUpdate()
{
	for (ComponentRefBase& component : _components)
	{
		if (component.IsValid() && component.Resolve()->IsEnabled())
			component.Resolve()->FixedUpdate();
	}

	for (ComponentRef<MonoBehaviour>& script : _scripts)
	{
		if (script.Resolve()->IsEnabled())
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

void GameObject::OnEnable()
{
    for (ComponentRefBase& component : _components)
    {
        if (component.IsValid() && component.Resolve()->IsEnabled())
            component.Resolve()->OnEnable();
    }
    for (ComponentRef<MonoBehaviour>& script : _scripts)
    {
        if (script.Resolve()->IsEnabled())
            script.Resolve()->OnEnable();
    }
}

void GameObject::OnDisable()
{
    for (ComponentRefBase& component : _components)
    {
        if (component.IsValid() && component.Resolve()->IsEnabled())
            component.Resolve()->OnDisable();
    }
    for (ComponentRef<MonoBehaviour>& script : _scripts)
    {
        if (script.Resolve()->IsEnabled())
            script.Resolve()->OnDisable();
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

TransformRef GameObject::GetTransformRef()
{
    return GetFixedComponentRef<Transform>();
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
    for (ComponentType type : S_RendererTypes)
    {
        Component* component = GetFixedComponent(type);
        if (component != nullptr)
            return static_cast<Renderer*>(component);
    }

	return nullptr;
}

bool GameObject::IsRenderer(ComponentType componentType)
{
    for (ComponentType type : S_RendererTypes)
    {
        if (type == componentType)
            return true;
    }
	return false;
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

UIImage* GameObject::GetUIImage()
{
    Component* component = GetFixedComponent(ComponentType::UIImage);
    return static_cast<UIImage*>(component);
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

Component* GameObject::AddComponent(unique_ptr<Component> component)
{
    if (component == nullptr)
        return nullptr;

    const uint8 index = static_cast<uint8>(component->GetType());
    if (index < FIXED_COMPONENT_COUNT && _components[index].Resolve() != nullptr)
    {
        ASSERT(false, "Component already exists");
        return nullptr;
    }

    Guid::SetCurrentInstanceId(CUR_SCENE->GetInstanceId());
	GameObjectRef thisRef(_guid);
	component->SetGameObject(thisRef);
	Component* componentPtr = CUR_SCENE->AddComponent(thisRef, std::move(component));
    if (componentPtr == nullptr)
        return nullptr;

	GuidRef guidRef(componentPtr->GetGuid());
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

	componentPtr->Awake();
	if (_isActive && componentPtr->IsEnabled())
		componentPtr->OnEnable();

    return componentPtr;
}

bool GameObject::RemoveComponent(Component* component)
{
    if (component == nullptr || component->GetGameObject() != this)
        return false;

    return CUR_SCENE->RemoveComponent(component);
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
		parentActive = parentTransform->GetGameObject()->IsActiveInHierarchy();
	}
	UpdateActiveInHierarchy(parentActive, false);
}

void GameObject::UpdateActiveInHierarchy(bool parentActive, bool forceUpdate)
{
	bool newActive = parentActive && _localActive;
    if (forceUpdate == false && _isActive == newActive)
        return;

	bool activeChanged = _isActive != newActive;
    _isActive = newActive;

	if (forceUpdate == false && activeChanged)
	{
        if (newActive)
            OnEnable();
        else
            OnDisable();
	}
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

void GameObject::OnGUI()
{
    char nameBuffer[256] = {};
    strncpy_s(nameBuffer, _name.c_str(), _TRUNCATE);

    ImGui::PushID("GameObjectName");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Name");
    ImGui::SameLine();
    ImGui::SetCursorPosX(200.0f);
    if (ImGui::InputText("##value", nameBuffer, IM_ARRAYSIZE(nameBuffer)))
    {
        SetName(nameBuffer);
    }
    ImGui::PopID();

    int childCount = GetTransform()->GetChildCount();
	if (childCount > 0)
	{
        ImGui::Text("Children: %d", childCount);
	}
    OnGUIUtils::DrawUInt8("Layer", &_layerIndex, 1.f);
}

GameObjectRef GameObject::GetGameObjectRefByGuid(const Guid& guid)
{
    return GameObjectRef(guid);
}

GameObject* GameObject::Instantiate(GameObject* original, Transform* parent)
{
    return CUR_SCENE->Instantiate(original, parent);
}
