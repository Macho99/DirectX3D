#pragma once

class IInspectorDrawer
{
public:
    virtual ~IInspectorDrawer() {}
    virtual bool Draw(Component& c) = 0; // 변경되면 true

protected:
    float _dragSpeed = 0.1f;
};

template<class T>
class InspectorDrawer : public IInspectorDrawer
{
public:
    InspectorDrawer() {}
    ~InspectorDrawer() {}

    bool Draw(Component& component) override
    {
        T& derived = static_cast<T&>(component);
        return DrawImpl(derived);
    }

    virtual bool DrawImpl(T& component) = 0;
};

