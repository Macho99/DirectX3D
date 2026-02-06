#include "pch.h"
#include "ComponentRef.h"

#include "Scene.h"         // GetComponentSlotManager, Scene 정의
#include "SlotManager.h"   // SlotManager<T>
#include "Component.h"     // Component

#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include "TessTerrain.h"
#include "MonoBehaviour.h"

template<class T>
T* ComponentRef<T>::Resolve() const
{
    if (!IsValid())
        return nullptr;

    SlotManager<Component>* manager = CUR_SCENE->GetComponentSlotManager();

    if (!cached.IsValid())
        cached = manager->FindHandle(guid);

    Component* p = manager->Resolve(cached);

#ifdef _DEBUG
    return dynamic_cast<T*>(p);
#else
    return static_cast<T*>(p);
#endif
}

// 명시적 인스턴스화
template class ComponentRef<Component>;
template class ComponentRef<Transform>;
template class ComponentRef<Camera>;
template class ComponentRef<Light>;
template class ComponentRef<TessTerrain>;
template class ComponentRef<MonoBehaviour>;