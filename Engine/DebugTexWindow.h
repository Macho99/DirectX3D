#pragma once
#include "EditorWindow.h"
class DebugTexWindow : public EditorWindow
{
    using Super = EditorWindow;

public:
    DebugTexWindow(wstring windowName, shared_ptr<Texture> debugTexture);
    ~DebugTexWindow();

    wstring GetName() const { return _windowName; }
protected:
    void OnGUI() override;

private:
    wstring _windowName;
    shared_ptr<Texture> _debugTexture;
};

