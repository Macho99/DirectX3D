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
        SlotManager<Component>& manager = CUR_SCENE->GetComponentSlotManager();
        if (!cached.IsValid())
            cached = manager.FindHandle(guid);
        Component* p = manager.Resolve(cached);
        
        return static_cast<T*>(p);
    }
};

using TransformRef = ComponentRef<class Transform>;
using ComponentRefBase = ComponentRef<class Component>;