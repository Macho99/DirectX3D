#pragma once
#include "Scene.h"

class SceneManager
{
	DECLARE_SINGLE(SceneManager);

public:
	void Init() { _currentScene->Start(); }
	void OnDestroy();
	void Update();

	template<typename T>
	void ChangeScene(shared_ptr<T> scene)
	{
        _currentScene->OnDestroy();
		_currentScene = scene;
		scene->Start();
	}
	
	Scene* GetCurrentScene() { return _currentScene.get(); }
    shared_ptr<Scene> GetCurrentSceneShared() { return _currentScene; }

private:
	shared_ptr<Scene> _currentScene = make_shared<Scene>();
};

