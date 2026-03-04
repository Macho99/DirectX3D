#pragma once

class InspectorDrawer
{
public:
    virtual ~InspectorDrawer() {}

protected:
    float _dragSpeed = 0.1f;
};