#pragma once
#include "InspectorDrawer.h"

class ResourceDrawerBase : public InspectorDrawer
{
public:
    virtual bool Draw(ResourceBase& resource) = 0;
};

template<class T>
class ResourceDrawer : public ResourceDrawerBase
{
public:
    ResourceDrawer() {}
    ~ResourceDrawer() {}
    bool Draw(ResourceBase& resource) override
    {
        T& derived = static_cast<T&>(resource);
        return DrawImpl(derived);
    }
    virtual bool DrawImpl(T& resource) = 0;
};