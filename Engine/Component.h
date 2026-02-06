#pragma once
class GameObject;
class Transform;
#include "GameObjectRef.h"

enum class ComponentType : uint8
{
	Transform,
	MeshRenderer,
	ModelRenderer,
	Camera,
	Animator, 
	Light,
	Collider,
	Terrain,
    TessTerrain,
	Button,
	Billboard,
	SnowBillboard,
	ParticleSystem,
	FoliageController,
    GrassRenderer,
	// ...
	Script,

	End,
};

enum
{
	FIXED_COMPONENT_COUNT = static_cast<uint8>(ComponentType::End) - 1
};

class Component
{
public:
	Component(ComponentType type);
	virtual ~Component();

	virtual void Awake() { }
	virtual void Start() { }	
	virtual void Update() { }
	virtual void LateUpdate() { }
	virtual void FixedUpdate() { }
    virtual void OnDestroy() {}

public:
	ComponentType GetType() { return _type; }

	GameObject* GetGameObject();
    GameObjectRef GetGameObjectRef() const { return _gameObject; }
	Transform* GetTransform();
    Guid GetGuid() const { return _guid; }
	void SetGuid(const Guid& guid) { _guid = guid; }

private:
	friend class GameObject;
	void SetGameObject(const GameObjectRef& gameObject) { _gameObject = gameObject; }

protected:
	ComponentType _type;
	GameObjectRef _gameObject;
    Guid _guid;
};

