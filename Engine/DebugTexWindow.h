#pragma once
#include "EditorWindow.h"
class DebugTexWindow : public EditorWindow
{
    using Super = EditorWindow;

public:
    DebugTexWindow(wstring windowName, function<Texture*()> getDebugTexture);
    ~DebugTexWindow();

    wstring GetName() const { return _windowName; }
protected:
    void OnGUI() override;

private:
    wstring _windowName;
    function<Texture*()> _getDebugTexture;
};

