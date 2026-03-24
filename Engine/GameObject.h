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

	void OnInspectorFocus();
    void OnInspectorFocusLost();

	Component* GetFixedComponent(ComponentType type);

	template<class T>
    ComponentRef<T> GetFixedComponentRef();

	Transform* GetTransform();
	Camera* GetCamera();
	Light* GetLight();
	MeshRenderer* GetMeshRenderer();
	ModelRenderer* GetModelRenderer();
	ModelAnimator* GetModelAnimator();
	Renderer* GetRenderer();
	BaseCollider* GetCollider();
	Terrain* GetTerrain();
	Button* GetButton();
	Billboard* GetBillboard();
	SnowBillboard* GetSnowBillboard();

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

	string GetName() const { return _name; }
	void SetName(string name) { _name = name; }
	bool IsActive() const { return _isActive; }
    bool IsLocallyActive() const { return _localActive; }
	void SetActive(bool active);
    void UpdateActiveInHierarchy(bool parentActive, bool forceUpdate = false);

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
