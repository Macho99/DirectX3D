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
	AABBBoxCollider,
    OBBBoxCollider,
    SphereCollider,
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
	Component();
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

	virtual bool OnGUI();

private:
	friend class GameObject;
	void SetGameObject(const GameObjectRef& gameObject) { _gameObject = gameObject; }

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::make_nvp("GameObject", _gameObject));
        ar(cereal::make_nvp("Guid", _guid));
    }

protected:
	ComponentType _type;
	GameObjectRef _gameObject;
    Guid _guid;
};

