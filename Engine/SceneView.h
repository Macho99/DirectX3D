#pragma once
#include "EditorWindow.h"
class SceneView : public EditorWindow
{
    using Super = EditorWindow;
public:
    SceneView();
    ~SceneView();

protected:
    void OnGUI() override;
};

