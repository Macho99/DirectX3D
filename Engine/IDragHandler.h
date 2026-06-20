#pragma once

struct DragEvent
{
    Vec2 delta;
};

class IDragHandler
{
public:
    virtual ~IDragHandler() = default;
    virtual void OnBeginDrag() = 0;
    virtual void OnDrag(DragEvent delta) = 0;
    virtual void OnEndDrag() = 0;
};

