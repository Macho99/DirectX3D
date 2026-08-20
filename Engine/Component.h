#pragma once
class GameObject;
class Transform;
#include "GameObjectRef.h"
#include "ComponentRegistry.h"
#include "cereal/archives/json.hpp"
#include <sstream>
#include <wrl/client.h>

// 절대 중간에 추가 및 삭제하지 말것
// 씬 직렬화된거 틀어짐
// 끝에 새로 추가하는 것만 가능
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
	ModelSocketFollower,
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
    friend class Scene;
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
	void InvokeStart();

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
	bool _isStarted = false;
};

template<class T>
bool MatchComponentType(Component* component)
{
    return dynamic_cast<T*>(component) != nullptr;
}

template<class T>
void CopyComponentBySerialization(Component* source, Component* target)
{
    std::stringstream buffer;
    {
        cereal::JSONOutputArchive archive(buffer);
        archive(cereal::make_nvp("Component", *static_cast<T*>(source)));
    }
    buffer.seekg(0);
    {
        cereal::JSONInputArchive archive(buffer);
        archive(cereal::make_nvp("Component", *static_cast<T*>(target)));
    }
}

