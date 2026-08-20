#pragma once
#include "Component.h"
#include "Transform.h"
#include "Light.h"
#include <wrl/client.h>
#include <type_traits>
#include "cereal/types/array.hpp"
#include "cereal/types/utility.hpp"

class MonoBehaviour;
class Transform;
class Camera;
class Light;
class MeshRenderer;
class ModelRenderer;
class ModelAnimator;
class BaseCollider;
class Terrain;
class Button;
class Billboard;
class SnowBillboard;
class Renderer;
class UIImage;

class GameObject
{
public:
    GameObject(string name = "GameObject");
    ~GameObject();

    void Awake();
    void Update();
    void LateUpdate();
    void FixedUpdate();
    void OnDestroy();

    void OnEnable();
    void OnDisable();

    void OnInspectorFocus();
    void OnInspectorFocusLost();

    Component* GetFixedComponent(ComponentType type);

    template<class T>
    ComponentRef<T> GetFixedComponentRef();

    template<class T>
    T* GetFixedComponent();

    template<class T>
    T* GetComponent();

    template<class T>
    T* GetComponentInChildren();

    Transform* GetTransform();
    TransformRef GetTransformRef();
    Camera* GetCamera();
    Light* GetLight();
    MeshRenderer* GetMeshRenderer();
    ModelRenderer* GetModelRenderer();
    ModelAnimator* GetModelAnimator();
    Renderer* GetRenderer();
    static bool IsRenderer(ComponentType componentType);
    BaseCollider* GetCollider();
    Terrain* GetTerrain();
    Button* GetButton();
    UIImage* GetUIImage();
    Billboard* GetBillboard();
    SnowBillboard* GetSnowBillboard();

    template<class T>
    T* AddComponent();

    template<class T>
    T* AddComponent(unique_ptr<T> component);

    Component* AddComponent(unique_ptr<Component> component);
    bool RemoveComponent(Component* component);
    array<ComponentRefBase, FIXED_COMPONENT_COUNT>& GetAllFixedComponents() { return _components; }
    vector<ComponentRef<MonoBehaviour>>& GetScripts() { return _scripts; }

    void SetLayerIndex(uint8 layer) { _layerIndex = layer; }
    uint8 GetLayerIndex() const { return _layerIndex; }

    template<typename T>
    T* GetFixedComponent(ComponentType type)
    {
        return static_cast<T*>(GetFixedComponent(type));
    }

    template<typename T>
    T* GetScriptComponent();

    string GetName() const { return _name; }
    void SetName(string name) { _name = name; }
    bool IsActiveInHierarchy() const { return _isActive; }
    bool IsActiveInLocal() const { return _localActive; }
    void SetActive(bool active);
    void UpdateActiveInHierarchy(bool parentActive, bool forceUpdate = false);

    void OnGUI();

    Guid GetGuid() const { return _guid; }
    void SetGuid(const Guid& guid) { _guid = guid; }

    static GameObjectRef GetGameObjectRefByGuid(const Guid& guid);
    static GameObject* Instantiate(GameObject* original, Transform* parent = nullptr);

    template<class T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
    static T* Instantiate(T* original, Transform* parent = nullptr)
    {
        if (original == nullptr)
            return nullptr;

        GameObject* clone = Instantiate(original->GetGameObject(), parent);
        return clone != nullptr ? clone->GetComponent<T>() : nullptr;
    }

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::make_nvp("Guid", _guid));
        ar(cereal::make_nvp("Name", _name));
        ar(cereal::make_nvp("LocalActive", _localActive));
        ar(cereal::make_nvp("LayerIndex", _layerIndex));

        if (Archive::is_loading::value)
        {
            vector<pair<ComponentType, ComponentRefBase>> componentsVector;
            ar(cereal::make_nvp("Components", componentsVector));
            for (const auto& pair : componentsVector)
            {
                ComponentType type = pair.first;
                if (type >= ComponentType::End)
                {
                    ASSERT(false, "type out of range");
                    continue;
                }
                _components[static_cast<uint8>(type)] = pair.second;
            }
        }
        else
        {
            vector<pair<ComponentType, ComponentRefBase>> componentsVector;
            for (int i = 0; i < FIXED_COMPONENT_COUNT; ++i)
            {
                if (_components[i].IsValid())
                {
                    componentsVector.emplace_back(static_cast<ComponentType>(i), _components[i]);
                }
            }
            ar(cereal::make_nvp("Components", componentsVector));
        }
        ar(cereal::make_nvp("Scripts", _scripts));
    }

private:
    static vector<ComponentType> S_RendererTypes;
    bool _isActive = true;
    bool _localActive = true;

    array<ComponentRefBase, FIXED_COMPONENT_COUNT> _components;
    vector<ComponentRef<MonoBehaviour>> _scripts;

    uint8 _layerIndex = 0;
    string _name;
    Guid _guid;
};

template<class T>
inline ComponentRef<T> GameObject::GetFixedComponentRef()
{
    ComponentType type = T::StaticType;
    if (type >= ComponentType::End)
    {
        ASSERT(false, "type out of range");
        return ComponentRef<T>();
    }
    return ComponentRef<T>(_components[static_cast<uint8>(type)]);
}

template<class T>
inline T* GameObject::GetFixedComponent()
{
    ComponentType type = T::StaticType;
    if (type >= ComponentType::End)
    {
        ASSERT(false, "type out of range");
        return nullptr;
    }
    return static_cast<T*>(_components[static_cast<uint8>(type)].Resolve());
}

template<class T>
inline T* GameObject::GetComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    if constexpr (std::is_base_of_v<MonoBehaviour, T>)
        return GetScriptComponent<T>();
    else
        return GetFixedComponent<T>();
}

template<class T>
inline T* GameObject::GetComponentInChildren()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    T* component = GetComponent<T>();
    if (component != nullptr)
        return component;

    Transform* transform = GetTransform();
    if (transform == nullptr)
        return nullptr;

    for (const TransformRef& childRef : transform->GetChildren())
    {
        Transform* childTransform = childRef.Resolve();
        if (childTransform == nullptr)
            continue;

        GameObject* childGameObject = childTransform->GetGameObject();
        if (childGameObject == nullptr)
            continue;

        component = childGameObject->GetComponentInChildren<T>();
        if (component != nullptr)
            return component;
    }

    return nullptr;
}

template<class T>
inline T* GameObject::AddComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    if constexpr (!std::is_base_of_v<MonoBehaviour, T>)
    {
        ComponentType type = T::StaticType;
        if (type >= ComponentType::Script)
        {
            ASSERT(false, "type out of range");
            return nullptr;
        }
    }

    unique_ptr<T> component = make_unique<T>();
    return static_cast<T*>(AddComponent(std::move(component)));
}

template<class T>
inline T* GameObject::AddComponent(unique_ptr<T> component)
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    unique_ptr<Component> baseComponent = std::move(component);
    return static_cast<T*>(AddComponent(std::move(baseComponent)));
}

template<class T>
inline T* GameObject::GetScriptComponent()
{
    for (const auto& scriptRef : _scripts)
    {
        T* script = dynamic_cast<T*>(scriptRef.Resolve());
        if (script != nullptr)
            return script;
    }
    return nullptr;
}
