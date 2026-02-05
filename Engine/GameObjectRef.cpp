#include "pch.h"
#include "GameObjectRef.h"
#include "SlotManager.h"

GameObject* GameObjectRef::Resolve() const
{
    SlotManager<GameObject>* manager = CUR_SCENE->GetGameObjectSlotManager();
    if (!cached.IsValid())
        cached = manager->FindHandle(guid);
    GameObject* p = manager->Resolve(cached);
    return p;
}
