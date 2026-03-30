#pragma once
#include "Component.h"
class NavMesh : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(NavMesh)
public:
    NavMesh();
    ~NavMesh();

    virtual bool OnGUI() override;
};

