#pragma once
#include "ComponentDrawer.h"

class TransformDrawer : public ComponentDrawer<Transform>
{
public:
    TransformDrawer() {}
    ~TransformDrawer() {}

    virtual bool DrawImpl(Transform& component) override;
};

