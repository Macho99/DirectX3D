#pragma once
#include "EditorWindow.h"
class SceneView : public EditorWindow
{
    using Super = EditorWindow;
public:
    SceneView();
    ~SceneView();

public:
    void Init() override;
    void OnGUI() override;
};

