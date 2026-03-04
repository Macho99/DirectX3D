#pragma once
#include "InspectorDrawer.h"

class ComponentDrawerBase : public InspectorDrawer
{
public:
    virtual bool Draw(Component& component) = 0;
};

template<class T>
class ComponentDrawer : public ComponentDrawerBase
{
public:
    ComponentDrawer() {}
    ~ComponentDrawer() {}

    bool Draw(Component& component) override
    {
        T& derived = static_cast<T&>(component);
        return DrawImpl(derived);
    }

    virtual bool DrawImpl(T& component) = 0;
};