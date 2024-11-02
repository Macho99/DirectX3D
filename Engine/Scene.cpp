#include "pch.h"
#include "Scene.h"

void Scene::Start()
{
	auto copys = _gameObjects;
	for (shared_ptr<GameObject> obj : copys)
	{
		obj->Start();
	}
}

void Scene::Update()
{
	auto copys = _gameObjects;
	for (shared_ptr<GameObject> obj : copys)
	{
		obj->Update();
	}	
	
	// INSTANCING
	vector<shared_ptr<GameObject>> temp;
	temp.insert(temp.end(), _gameObjects.begin(), _gameObjects.end());
	INSTANCING->Render(temp);
}

void Scene::LateUpdate()
{
	auto copys = _gameObjects;
	for (shared_ptr<GameObject> obj : copys)
	{
		obj->LateUpdate();
	}
}

void Scene::Add(shared_ptr<GameObject> gameObject)
{
	_gameObjects.insert(gameObject);
	if (gameObject->GetFixedComponent(ComponentType::Camera) != nullptr)
	{
		_cameras.insert(gameObject);
	}
	if (gameObject->GetFixedComponent(ComponentType::Light) != nullptr)
	{
		_lights.insert(gameObject);
	}
}

void Scene::Remove(shared_ptr<GameObject> gameObject)
{
	_gameObjects.erase(gameObject);
	_cameras.erase(gameObject);
	_lights.erase(gameObject);
}
