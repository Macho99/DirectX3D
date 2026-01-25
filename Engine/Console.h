#pragma once
#include "EditorWindow.h"
class Console :
    public EditorWindow
{
    using Super = EditorWindow;
public:
    Console();
    ~Console();
    void Init() override;

protected:
    void OnGUI() override;
};

