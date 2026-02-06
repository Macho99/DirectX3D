#pragma once
#include "GuidRef.h"

template<class T>
struct ComponentRef : public GuidRef
{
public:
    ComponentRef() : GuidRef() {}
    ComponentRef(const Guid& guid) : GuidRef(guid) {}
    ComponentRef(const GuidRef& guidRef) : GuidRef(guidRef) {}
    using GuidRef::operator==;
    ~ComponentRef() {}

    T* Resolve() const;
};

using TransformRef = ComponentRef<class Transform>;
using ComponentRefBase = ComponentRef<class Component>;