#include "pch.h"
#include "SceneManager.h"

void SceneManager::OnDestroy()
{
    if (_currentScene)
        _currentScene->OnDestroy();
	_currentScene.reset();
}

void SceneManager::Update()
{
	if (_currentScene == nullptr)
		return;

	_currentScene->Update();
	_currentScene->LateUpdate();
	_currentScene->Render();
	_currentScene->CleanUpRemoveLists();
}
