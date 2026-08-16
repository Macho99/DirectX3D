#pragma once
#include "GuidRef.h"

class Component;

struct ComponentRefResolver : public GuidRef
{
public:
    ComponentRefResolver() : GuidRef() {}

protected:
    ComponentRefResolver(const Guid& guid) : GuidRef(guid) {}
    ComponentRefResolver(const GuidRef& guidRef) : GuidRef(guidRef) {}

    Component* ResolveComponent() const;
};

template<class T>
struct ComponentRef : public ComponentRefResolver
{
public:
    ComponentRef() : ComponentRefResolver() {}
    ComponentRef(decltype(nullptr)) : ComponentRefResolver() {}
    ComponentRef(const T* component)
        : ComponentRefResolver(component != nullptr ? component->GetGuid() : Guid())
    {
    }

private:
    ComponentRef(const Guid& guid) : ComponentRefResolver(guid) {}
    ComponentRef(const GuidRef& guidRef) : ComponentRefResolver(guidRef) {}

public:
    using ComponentRefResolver::operator==;
    ~ComponentRef() {}

    T* Resolve() const
    {
        Component* component = ResolveComponent();

#ifdef _DEBUG
        return dynamic_cast<T*>(component);
#else
        return static_cast<T*>(component);
#endif
    }

    friend class GameObject;
};

using TransformRef = ComponentRef<class Transform>;
using RectTransformRef = ComponentRef<class RectTransform>;
using ComponentRefBase = ComponentRef<class Component>;
using UIRendererRef = ComponentRef<class UIRenderer>;
