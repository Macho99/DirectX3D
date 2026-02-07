#pragma once
#include "EditorWindow.h"
#include "ImGuizmo.h"

struct GizmoUIState
{
    ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE mode = ImGuizmo::LOCAL;

    bool useSnap = false;
    float snapMove = 0.5f;
    float snapRotate = 15.0f;
    float snapScale = 0.1f;

    bool show = true;
};

class SceneView : public EditorWindow
{
    using Super = EditorWindow;
public:
    SceneView();
    ~SceneView();

protected:
    void OnGUI() override;

private:
    void DrawSceneViewGizmoOverlay();

private:
    GizmoUIState g_gizmo;
};

