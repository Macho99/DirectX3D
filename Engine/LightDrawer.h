#pragma once
#include "ComponentDrawer.h"

class LightDrawer : public ComponentDrawer<class Light>
{
    virtual bool DrawImpl(Light& component) override;
};

