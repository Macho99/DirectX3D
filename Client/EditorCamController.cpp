#include "pch.h"
#include "EditorCamController.h"
#include "ThirdPersonCamMove.h"
#include "CameraMove.h"


void EditorCamController::Awake()
{
    _initLocalPos = GetGameObject()->GetTransform()->GetLocalPosition();
}

void EditorCamController::Update()
{
    if (INPUT->IsMouseCaptured() == false && INPUT->GetButtonDown(KEY_TYPE::LBUTTON) && INPUT->IsMouseOnUI() == false)
    {
        GetGameObject()->GetScriptComponent<ThirdPersonCamMove>()->SetEnabled(true);
        GetGameObject()->GetScriptComponent<CameraMove>()->SetEnabled(false);
        GetGameObject()->GetTransform()->SetLocalRotation(Vec3::Zero);
        GetGameObject()->GetTransform()->SetLocalPosition(_initLocalPos);
        INPUT->CaptureMouseCursor();
        return;
    }

    if (GAME->GetGameDesc().isEditor && INPUT->IsMouseCaptured() == false && INPUT->GetButtonDown(KEY_TYPE::ESC))
    {
        GetGameObject()->GetScriptComponent<ThirdPersonCamMove>()->SetEnabled(false);
        GetGameObject()->GetScriptComponent<CameraMove>()->SetEnabled(true);
        GetTransform()->GetParent()->SetLocalRotation(Vec3::Zero);
        return;
    }
}
