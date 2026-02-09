#pragma once
#include "GameObjectRef.h"
#include "ComponentRef.h"

class Sky;
class Camera;
class Transform;
struct Guid;

template<class T>
class SlotManager;

class Scene
{
public:
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

	GameObjectRef Add(unique_ptr<GameObject> gameObject);    
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

    SlotManager<GameObject>* GetGameObjectSlotManager() { return _gameObjectSlotManager.get(); }
    SlotManager<Component>* GetComponentSlotManager() { return _componentSlotManager.get(); }

    uint64 GetInstanceId() const { return _instanceId; }

private:
	GameObjectRef Add(GuidRef guidRef);

private:
    vector<TransformRef> _rootObjects;
	GameObjectRefSet _gameObjects;
	GameObjectRefSet _cameras;	// Cache Camera
	GameObjectRefSet _lights;	// Cache Light
	shared_ptr<Sky> _sky;
	vector<GameObjectRef> _removeLists;

    unique_ptr<SlotManager<GameObject>> _gameObjectSlotManager;
    unique_ptr<SlotManager<Component>> _componentSlotManager;

	uint64 _instanceId;
};