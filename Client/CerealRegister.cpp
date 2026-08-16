#include "pch.h"
#include "CerealRegister.h"
#include "MonoBehaviour.h"
#include "CameraMove.h"
#include "ThirdPersonCamMove.h"
#include "PlayerAnimEventHandler.h"
#include "ServerConnect.h"
#include "EditorCamController.h"
#include "GameManager.h"
#include "Player.h"

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

CEREAL_REGISTER_TYPE(GameManager);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, GameManager);

CEREAL_REGISTER_TYPE(Player);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, Player);

void RegisterClientComponents()
{
    CameraMove::EnsureAutoRegister();
    ThirdPersonCamMove::EnsureAutoRegister();
    PlayerAnimEventHandler::EnsureAutoRegister();
    ServerConnect::EnsureAutoRegister();
    EditorCamController::EnsureAutoRegister();
    GameManager::EnsureAutoRegister();
    Player::EnsureAutoRegister();
}
