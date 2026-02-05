#pragma once

#include "ComponentRef.h"
#include "GameObjectRef.h"

class Sky;
class Camera;
class Transform;
class Guid;

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

	virtual void Add(unique_ptr<GameObject> gameObject);
	virtual void Remove(unique_ptr<GameObject> gameObject);
	void CleanUpRemoveLists();
	void RemoveGameObjectRecur(const shared_ptr<GameObject>& gameObject);

	GameObjectRefSet& GetObjects() { return _gameObjects; }
	GameObjectRefSet& GetCameras() { return _cameras; }
	GameObject* GetMainCamera();
	GameObject* GetUICamera();
	GameObject* GetLight() { return _lights.empty() ? nullptr : _lights.begin()->Resolve(); }

	void PickUI();
	shared_ptr<GameObject> Pick(int32 screenX, int32 screenY);

	void SetSky(shared_ptr<Sky> sky) { _sky = sky; }

	void CheckCollision();
	bool IsInScene(const GameObjectRef& ref) { return _gameObjects.find(ref) != _gameObjects.end(); }
	vector<ComponentRef<Transform>>& GetRootObjects() { return _rootObjects; }

    SlotManager<GameObject>* GetGameObjectSlotManager() { return _gameObjectSlotManager.get(); }
    SlotManager<Component>* GetComponentSlotManager() { return _componentSlotManager.get(); }

private:
    vector<TransformRef> _rootObjects;
	GameObjectRefSet _gameObjects;
	GameObjectRefSet _cameras;	// Cache Camera
	GameObjectRefSet _lights;	// Cache Light
	shared_ptr<Sky> _sky;
	vector<GameObjectRef> _removeLists;

    unique_ptr<SlotManager<GameObject>> _gameObjectSlotManager;
    unique_ptr<SlotManager<Component>> _componentSlotManager;
};

