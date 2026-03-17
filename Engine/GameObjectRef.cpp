#include "pch.h"
#include "GameObjectRef.h"
#include "SlotManager.h"

GameObject* GameObjectRef::Resolve() const
{
    if (!IsValid())
        return nullptr;

    SlotManager<GameObject>* manager = CUR_SCENE->GetGameObjectSlotManager();
    uint64 sceneInstanceId = CUR_SCENE->GetInstanceId();

    if (!cached.IsValid() || cachedSceneInstanceId != sceneInstanceId)
    {
        cached = manager->FindHandle(guid);
        cachedSceneInstanceId = sceneInstanceId;
    }

    GameObject* p = manager->Resolve(cached);
    return p;
}
