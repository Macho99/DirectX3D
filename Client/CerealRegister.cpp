#include "pch.h"
#include "CerealRegister.h"
#include "MonoBehaviour.h"
#include "CameraMove.h"
#include "ThirdPersonCamMove.h"
#include "PlayerAnimEventHandler.h"

CEREAL_REGISTER_TYPE(CameraMove);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, CameraMove);

CEREAL_REGISTER_TYPE(ThirdPersonCamMove);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, ThirdPersonCamMove);

CEREAL_REGISTER_TYPE(PlayerAnimEventHandler);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, PlayerAnimEventHandler);


void RegisterClientComponents()
{
    CameraMove::EnsureAutoRegister();
    ThirdPersonCamMove::EnsureAutoRegister();
    PlayerAnimEventHandler::EnsureAutoRegister();
}