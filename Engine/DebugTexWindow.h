#pragma once
#include "EditorWindow.h"
class DebugTexWindow : public EditorWindow
{
    using Super = EditorWindow;

public:
    DebugTexWindow(string windowName, function<Texture*()> getDebugTexture);
    ~DebugTexWindow();

protected:
    void OnGUI() override;

private:
    function<Texture*()> _getDebugTexture;
};

