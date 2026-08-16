#include "pch.h"
#include "ComponentRef.h"

#include "Scene.h"
#include "SlotManager.h"   // SlotManager<T>
#include "Component.h"     // Component

Component* ComponentRefResolver::ResolveComponent() const
{
    if (!IsValid())
        return nullptr;

    SlotManager<Component>* manager = CUR_SCENE->GetComponentSlotManager();
    uint64 sceneInstanceId = CUR_SCENE->GetInstanceId();

    if (!cached.IsValid() || cachedSceneInstanceId != sceneInstanceId)
    {
        cached = manager->FindHandle(guid);
        cachedSceneInstanceId = sceneInstanceId;
    }

    return manager->Resolve(cached);
}
