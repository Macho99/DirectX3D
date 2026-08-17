#include "pch.h"
#include "EditorCamController.h"
#include "ThirdPersonCamMove.h"
#include "CameraMove.h"
#include "TessTerrain.h"
#include "ImGuizmo.h"

void EditorCamController::Awake()
{
    _initLocalPos = GetGameObject()->GetTransform()->GetLocalPosition();
    _terrain = CUR_SCENE->FindComponent<TessTerrain>();
    FindCamMove();
}

void EditorCamController::Update()
{
    if (_terrain.Resolve()->GetEditMode() != TessTerrain::EditMode::None)
    {
        return;
    }

    const bool isMouseOverGizmo = GAME->GetGameDesc().isEditor && ImGuizmo::IsOver();
    if (INPUT->IsMouseCaptured() == false && INPUT->GetButtonUp(KEY_TYPE::LBUTTON) &&
        INPUT->IsMouseOnUI() == false && isMouseOverGizmo == false)
    {
        ThirdPersonCamMove* charCamMove = _charCamMove.Resolve();
        if (charCamMove != nullptr)
            charCamMove->SetEnabled(true);

        _editorCamMove.Resolve()->SetEnabled(false);
        GetGameObject()->GetTransform()->SetLocalRotation(Vec3::Zero);
        GetGameObject()->GetTransform()->SetLocalPosition(_initLocalPos);
        INPUT->CaptureMouseCursor();
        return;
    }

    if (GAME->GetGameDesc().isEditor && INPUT->IsMouseCaptured() == false && INPUT->GetButtonDown(KEY_TYPE::ESC))
    {
        ThirdPersonCamMove* charCamMove = _charCamMove.Resolve();
        if(charCamMove != nullptr)
            charCamMove->SetEnabled(false);

        _editorCamMove.Resolve()->SetEnabled(true);
        GetTransform()->GetParent()->SetLocalRotation(Vec3::Zero);
        return;
    }
}

void EditorCamController::FindCamMove()
{
    _editorCamMove = GetGameObject()->GetScriptComponent<CameraMove>();
    _charCamMove = GetGameObject()->GetScriptComponent<ThirdPersonCamMove>();
}
