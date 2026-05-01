#pragma once
#include "GameObjectRef.h"
#include "ComponentRef.h"
#include "SlotManager.h"
#include "cereal/types/unordered_set.hpp"
#include "Sky.h"
#include "GameObject.h"

class Camera;
class Transform;
struct Guid;

class Scene : public ResourceBase
{
	using Super = ResourceBase;
public:
	static constexpr ResourceType StaticType = ResourceType::Scene;
	Scene();
	virtual ~Scene();

public:
	virtual void Start();
    virtual void OnDestroy();
	virtual void Update();
	virtual void LateUpdate();

	virtual void Render();
	void RenderGameCamera(Camera* cam);
	void RenderUICamera(Camera* cam);

	GameObjectRef Add(string name);
	GuidRef AddComponent(GameObjectRef objRef, unique_ptr<Component> component);
	virtual void Remove(GameObjectRef gameObjectRef);
	void CleanUpRemoveLists();
	void RemoveGameObjectRecur(const GameObjectRef& gameObjectRef);

	GameObjectRefSet& GetObjects() { return _gameObjects; }
	GameObjectRefSet& GetCameras() { return _cameras; }
	GameObject* GetMainCamera();
	GameObject* GetUICamera();
	GameObject* GetLight() { return _lights.empty() ? nullptr : _lights.begin()->Resolve(); }

	void PickUI();
	GameObject* Pick(int32 screenX, int32 screenY);

	void SetSky(shared_ptr<Sky> sky) { _sky = sky; }

	void CheckCollision();
	bool IsInScene(const GameObjectRef& ref) { return _gameObjects.find(ref) != _gameObjects.end(); }
	vector<TransformRef>& GetRootObjects() { return _rootObjects; }

    SlotManager<GameObject>* GetGameObjectSlotManager() { return &_gameObjectSlotManager; }
    SlotManager<Component>* GetComponentSlotManager() { return &_componentSlotManager; }

    uint64 GetInstanceId() const { return _instanceId; }

    template<class T>
    ComponentRef<T> FindComponentRef() const
    {
        for (const auto& gameObjectRef : _gameObjects)
        {
            GameObject* gameObject = gameObjectRef.Resolve();
            if (gameObject)
            {
                ComponentRef<T> componentRef = gameObject->GetFixedComponentRef<T>();
                if (componentRef.IsValid())
                {
                    return componentRef;
                }
            }
        }
        return ComponentRef<T>();
    }

private:
	GameObjectRef Add(GuidRef guidRef);

public:
    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
			cereal::make_nvp("RootObjects", _rootObjects),
            cereal::make_nvp("GameObjects", _gameObjects),
            cereal::make_nvp("Cameras", _cameras),
            cereal::make_nvp("Lights", _lights),
            cereal::make_nvp("GameObjectSlots", _gameObjectSlotManager),
            cereal::make_nvp("ComponentSlots", _componentSlotManager),
            cereal::make_nvp("Sky", _sky)
		);
    }

private:
    vector<TransformRef> _rootObjects;
	GameObjectRefSet _gameObjects;
	GameObjectRefSet _cameras;	// Cache Camera
	GameObjectRefSet _lights;	// Cache Light
	shared_ptr<Sky> _sky;
	vector<GameObjectRef> _removeLists;

    SlotManager<GameObject> _gameObjectSlotManager;
    SlotManager<Component> _componentSlotManager;

	uint64 _instanceId;
};