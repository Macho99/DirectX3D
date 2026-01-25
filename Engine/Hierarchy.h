#pragma once
#include "EditorWindow.h"
class Hierarchy : public EditorWindow
{
    using Super = EditorWindow;
public:
    Hierarchy() {}
    ~Hierarchy() {}
    void Init() override;

protected:
    void OnGUI() override;
};

