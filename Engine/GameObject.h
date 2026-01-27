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

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject(wstring name = L"GameObject");
	~GameObject();

	void Awake();
	void Start();
	void Update();
	void LateUpdate();
	void FixedUpdate();
    void OnDestroy();

	shared_ptr<Component> GetFixedComponent(ComponentType type);
	shared_ptr<Transform> GetTransform();
	shared_ptr<Camera> GetCamera();
	shared_ptr<Light> GetLight();
	shared_ptr<MeshRenderer> GetMeshRenderer();
	shared_ptr<ModelRenderer> GetModelRenderer();
	shared_ptr<ModelAnimator> GetModelAnimator();
	shared_ptr<Renderer> GetRenderer();
	shared_ptr<BaseCollider> GetCollider();
	shared_ptr<Terrain> GetTerrain();
	shared_ptr<Button> GetButton();
	shared_ptr<Billboard> GetBillboard();
	shared_ptr<SnowBillboard> GetSnowBillboard();

	void AddComponent(shared_ptr<Component> component);

	void SetLayerIndex(uint8 layer) { _layerIndex = layer; }
	uint8 GetLayerIndex() { return _layerIndex; }

	template<typename T>
	shared_ptr<T> GetFixedComponent(ComponentType type)
	{
		return static_pointer_cast<T>(GetFixedComponent(type));
	}

protected:
	array<shared_ptr<Component>, FIXED_COMPONENT_COUNT> _components;
	vector<shared_ptr<MonoBehaviour>> _scripts;

	uint8 _layerIndex = 0;
	wstring _name;
};

