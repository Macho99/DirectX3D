#pragma once
#include "Component.h"
#include "Light.h"
#include <wrl/client.h>
#include "cereal/types/array.hpp"
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
    void Start();
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
    ComponentRef<T> AddComponent();
    void AddComponent(unique_ptr<Component> component);
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

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::make_nvp("Guid", _guid));
        ar(cereal::make_nvp("Name", _name));
        ar(cereal::make_nvp("LocalActive", _localActive));
        ar(cereal::make_nvp("LayerIndex", _layerIndex));
        ar(cereal::make_nvp("Components", _components));
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
inline ComponentRef<T> GameObject::AddComponent()
{
    ComponentType type = T::StaticType;
    if (type >= ComponentType::End)
    {
        ASSERT(false, "type out of range");
        return ComponentRef<T>();
    }
    unique_ptr<T> component = make_unique<T>();
    T* componentPtr = component.get();
    AddComponent(std::move(component));
    return ComponentRef<T>(componentPtr);
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