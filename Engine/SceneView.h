#pragma once
#include "EditorWindow.h"
#include "ImGuizmo.h"

class Camera;
class Transform;
class RectTransform;
class Viewport;

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

public:
    void SetTransformGizmoOverride(const Matrix& matrix, function<void(const Matrix&)> onMatrixChanged);
    void ClearTransformGizmoOverride();

protected:
    void OnGUI() override;

private:
    void DrawSceneViewGizmoOverlay();
    void DrawTransformGizmo(Transform* selectedTransform, Camera* camera);
    void DrawRectTransformGizmo(RectTransform* selectedTransform, Camera* camera);
    Vec2 GetRectTransformDragDelta(RectTransform* rectTransform, Camera* camera, Viewport& viewport) const;

private:
    GizmoUIState g_gizmo;

private:
    bool _transformGizmoOverrided = false;
    Matrix _overrideMatrix;
    function<void(const Matrix&)> _onMatrixChanged;
};