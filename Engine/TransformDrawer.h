#pragma once
#include "InspectorDrawer.h"
class TransformDrawer : public InspectorDrawer<Transform>
{
public:
    TransformDrawer() {}
    ~TransformDrawer() {}

    virtual bool DrawImpl(Transform& component) override;
};

