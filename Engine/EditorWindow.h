#pragma once

class EditorManager;

class EditorWindow
{
public:
    EditorWindow(string windowName, bool enableMenu = true);
    virtual ~EditorWindow() {}

    virtual void Init(EditorManager* editorManager);
    void CheckAndOnGUI();

    string GetName() const { return _windowName; }
    bool IsMenuEnabled() const { return _enableMenu; }
protected:
    virtual void OnGUI() {}

public:
    bool IsOpen = true;
protected:
    string _windowName;
    bool _enableMenu;

    EditorManager* _editorManager = nullptr;
};

