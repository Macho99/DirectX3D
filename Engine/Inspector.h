#pragma once
#include "EditorWindow.h"
class Inspector :
    public EditorWindow
{
    using Super = EditorWindow;
public:
    Inspector();
    ~Inspector();

protected:
    void OnGUI() override;
};

