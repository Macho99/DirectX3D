#pragma once

class Sky;
class Camera;
class Transform;

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

	virtual void Add(shared_ptr<GameObject> gameObject);
	virtual void Remove(shared_ptr<GameObject> gameObject);
	void CleanUpRemoveLists();

	unordered_map<TransformID, shared_ptr<GameObject>>& GetObjects() { return _gameObjects; }
    unordered_set<shared_ptr<GameObject>>& GetCameras() { return _cameras; }
	shared_ptr<GameObject> GetMainCamera();
	shared_ptr<GameObject> GetUICamera();
	shared_ptr<GameObject> GetLight() { return _lights.empty() ? nullptr : *_lights.begin(); }

	void PickUI();
	shared_ptr<GameObject> Pick(int32 screenX, int32 screenY);

	void SetSky(shared_ptr<Sky> sky) { _sky = sky; }

	void CheckCollision();
	bool IsInScene(TransformID id) { return _gameObjects.find(id) != _gameObjects.end(); }
	vector<shared_ptr<Transform>>& GetRootObjects() { return _rootObjects; }
	bool TryGetTransform(TransformID id, OUT shared_ptr<Transform>& transform);

private:
    vector<shared_ptr<Transform>> _rootObjects;
	unordered_map<TransformID, shared_ptr<GameObject>> _gameObjects;
	// Cache Camera
	unordered_set<shared_ptr<GameObject>> _cameras;
	// Cache Light
	unordered_set<shared_ptr<GameObject>> _lights;
	shared_ptr<Sky> _sky;
	vector<shared_ptr<GameObject>> _removeLists;
};

