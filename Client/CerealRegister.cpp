#include "pch.h"
#include "CerealRegister.h"
#include "MonoBehaviour.h"
#include "CameraMove.h"
#include "ThirdPersonCamMove.h"
#include "PlayerAnimEventHandler.h"
#include "ServerConnect.h"
#include "EditorCamController.h"

CEREAL_REGISTER_TYPE(CameraMove);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, CameraMove);

CEREAL_REGISTER_TYPE(ThirdPersonCamMove);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, ThirdPersonCamMove);

CEREAL_REGISTER_TYPE(PlayerAnimEventHandler);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, PlayerAnimEventHandler);

CEREAL_REGISTER_TYPE(ServerConnect);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, ServerConnect);

CEREAL_REGISTER_TYPE(EditorCamController);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, EditorCamController);

void RegisterClientComponents()
{
    CameraMove::EnsureAutoRegister();
    ThirdPersonCamMove::EnsureAutoRegister();
    PlayerAnimEventHandler::EnsureAutoRegister();
    ServerConnect::EnsureAutoRegister();
    EditorCamController::EnsureAutoRegister();
}

template struct ComponentRef<CameraMove>;
template struct ComponentRef<ThirdPersonCamMove>;