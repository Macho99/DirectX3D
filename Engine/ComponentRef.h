#pragma once
#include "GuidRef.h"

template<class T>
struct ComponentRef : public GuidRef
{
public:
    ComponentRef() : GuidRef() {}

private:
    ComponentRef(const Guid& guid) : GuidRef(guid) {}
    ComponentRef(const GuidRef& guidRef) : GuidRef(guidRef) {}

public:
    using GuidRef::operator==;
    ~ComponentRef() {}

    T* Resolve() const;

    friend class GameObject;
};

using TransformRef = ComponentRef<class Transform>;
using ComponentRefBase = ComponentRef<class Component>;