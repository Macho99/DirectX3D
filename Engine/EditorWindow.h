#pragma once
class EditorWindow
{
public:
    EditorWindow() {}
    virtual ~EditorWindow() {}

    virtual void Init() {}
    void CheckAndOnGUI();

protected:
    virtual void OnGUI() {}
public:
    bool IsOpen = true;
};

