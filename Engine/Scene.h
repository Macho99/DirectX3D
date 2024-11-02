#pragma once
class Scene
{
public:
	virtual void Start();
	virtual void Update();
	virtual void LateUpdate();

	virtual void Add(shared_ptr<GameObject> gameObject);
	virtual void Remove(shared_ptr<GameObject> gameObject);

private:
	unordered_set<shared_ptr<GameObject>> _gameObjects;
	// Cache Camera
	unordered_set<shared_ptr<GameObject>> _cameras;
	// Cache Light
	unordered_set<shared_ptr<GameObject>> _lights;
};

