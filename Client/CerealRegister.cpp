#include "pch.h"
#include "CerealRegister.h"
#include "MonoBehaviour.h"
#include "CameraMove.h"

CEREAL_REGISTER_TYPE(CameraMove);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MonoBehaviour, CameraMove);