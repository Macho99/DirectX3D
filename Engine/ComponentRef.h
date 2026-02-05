#pragma once
#include "Component.h"
#include "GuidRef.h"

template<class T>
class ComponentRef : public GuidRef
{
public:
    ComponentRef(Guid guid) : GuidRef(guid) {}
    ~ComponentRef() {}

    T* Resolve() const
    {
        SlotManager<T>& manager = CUR_SCENE->GetComponentSlotManager();
        if (!cached.IsValid())
            cached = manager.FindHandle(guid);
        Component* p = manager.Resolve(cached);
        
        return dynamic_cast<T*>(p);
    }

    template<class Component>
    Component* Resolve() const
    {
        SlotManager<T>& manager = CUR_SCENE->GetComponentSlotManager();
        if (!cached.IsValid())
            cached = manager.FindHandle(guid);
        Component* p = manager.Resolve(cached);
        return p;
    }
};

using TransformRef = ComponentRef<class Transform>;