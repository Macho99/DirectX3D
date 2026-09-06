#include "pch.h"
#include "SceneManager.h"
#include "GpuProfiler.h"

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

    const auto cpuUpdateStart = GpuProfiler::CpuClock::now();
	_currentScene->Update();
	_currentScene->LateUpdate();
    GpuProfiler::Get().SetCpuUpdate(std::chrono::duration<double, std::milli>(GpuProfiler::CpuClock::now() - cpuUpdateStart).count());
	_currentScene->Render();
	_currentScene->CleanUpRemoveLists();
}
