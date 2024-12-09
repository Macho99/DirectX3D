#pragma once
class Scene
{
public:
	virtual void Start();
	virtual void Update();
	virtual void LateUpdate();

	virtual void Add(shared_ptr<GameObject> gameObject);
	virtual void Remove(shared_ptr<GameObject> gameObject);

	unordered_set<shared_ptr<GameObject>> GetObjects() { return _gameObjects; }
	shared_ptr<GameObject> GetCamera() { return _cameras.empty() ? nullptr : *_cameras.begin(); }
	shared_ptr<GameObject> GetLight() { return _lights.empty() ? nullptr : *_lights.begin(); }

	shared_ptr<GameObject> Pick(int32 screenX, int32 screenY);

private:
	unordered_set<shared_ptr<GameObject>> _gameObjects;
	// Cache Camera
	unordered_set<shared_ptr<GameObject>> _cameras;
	// Cache Light
	unordered_set<shared_ptr<GameObject>> _lights;
};

