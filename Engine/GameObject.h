#pragma once
#include "Component.h"
#include "Light.h"
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

	Component* GetFixedComponent(ComponentType type);
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
	void SetActive(bool active) { _isActive = active; }

    Guid GetGuid() const { return _guid; }
	void SetGuid(const Guid& guid) { _guid = guid; }

private:
	bool _isActive = true;
	
	array<ComponentRefBase, FIXED_COMPONENT_COUNT> _components;
	vector<ComponentRef<MonoBehaviour>> _scripts;

	uint8 _layerIndex = 0;
	string _name;
    Guid _guid;
};