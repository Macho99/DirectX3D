#pragma once

class SceneView;
class Hierarchy;
class Inspector;
class Console;

class EditorManager
{
public:
    EditorManager();
    ~EditorManager();
	static EditorManager* GetInstance()
	{
		static EditorManager s_instance;
		return &s_instance;
	}

public:
	void Init();
	void Update();
	void DrawDockSpace();
	void Render();
    void OnDestroy();

private:
    unique_ptr<SceneView> _sceneView;
    unique_ptr<Hierarchy> _hierarchy;
    unique_ptr<Inspector> _inspector;
    unique_ptr<Console> _console;
};