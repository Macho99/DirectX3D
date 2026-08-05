#include "pch.h"
#include "PlayerAnimEventHandler.h"
#include "AnimationImportSetting.h"

void PlayerAnimEventHandler::OnAnimationEvent(const AnimationEvent& animationEvent)
{
    if (animationEvent.eventName != "Attack")
        return;
    
    DBG->Log("Attack");
}
