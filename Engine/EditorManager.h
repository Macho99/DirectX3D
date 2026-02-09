#pragma once
#include "DirectoryWatcherWin.h"
class EditorWindow;

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

public:
    TransformRef GetSelectedTransform() const { return _selectedTransform; }
    void SetSelectedTransform(TransformRef& transformRef) { _selectedTransform = transformRef; }

private:
	TransformRef _selectedTransform;
	vector<unique_ptr<EditorWindow>> _editorWindows;
	DirectoryWatcherWin watcher;
};