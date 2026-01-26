#pragma once
#include "EditorWindow.h"

struct ImRect 
{ 
    ImVec2 Min, Max; 

    float GetHeight() const { return Max.y - Min.y; }
};

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

