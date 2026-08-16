#pragma once
class GameObject;
class Transform;
#include "GameObjectRef.h"
#include "ComponentRegistry.h"
#include <wrl/client.h>

enum class ComponentType : uint8
{
	Transform,
	MeshRenderer,
	ModelRenderer,
	Camera,
	ModelAnimator, 
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
    NavMesh,
    LineRenderer,
	NavAgent,
    SsrRenderer,
    Text,
    UIImage,
    VerticalLayoutGroup,
    HorizontalLayoutGroup,
    GridLayoutGroup,
    ScrollView,
	TargetFollower,
	TrailRenderer,
	InputText,
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
    virtual void OnDestroy() { }

    virtual void OnEnable() {}
    virtual void OnDisable() {}

	virtual void OnInspectorFocus() { }
	virtual void OnInspectorFocusLost() { }

    virtual int GetVersion() const { return 0; }

public:
	ComponentType GetType() const { return _type; }

	GameObject* GetGameObject();
    GameObjectRef GetGameObjectRef() const { return _gameObject; }
	Transform* GetTransform();
    TransformRef GetTransformRef();
	Guid GetGuid() const { return _guid; }
	void SetGuid(const Guid& guid) { _guid = guid; }
	bool IsEnabled() const { return _enabled; }
	bool IsActiveAndEnabled() const;
	void SetEnabled(bool enabled);

	virtual bool OnGUI();
    virtual void OnMenu();

private:
	friend class GameObject;
	void SetGameObject(const GameObjectRef& gameObject) { _gameObject = gameObject; }

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
		if (Archive::is_saving::value)
			_version = GetVersion();

		ar(CEREAL_NVP(_version));
        ar(cereal::make_nvp("GameObject", _gameObject));
        ar(cereal::make_nvp("Guid", _guid));

		if (Archive::is_saving::value)
		{
			ar(cereal::make_nvp("Enabled", _enabled));
		}
		else
		{
			// Scenes saved before component toggles existed have no Enabled field.
			try
			{
				ar(cereal::make_nvp("Enabled", _enabled));
			}
			catch (const cereal::Exception&)
			{
				_enabled = true;
			}
		}
    }

protected:
	int _version;
	ComponentType _type;
	GameObjectRef _gameObject;
    Guid _guid;
	bool _enabled = true;
};

