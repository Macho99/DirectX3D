#pragma once
#include "InspectorDrawer.h"

class MetaFileDrawerBase : public InspectorDrawer
{
public:
    virtual bool Draw(MetaFile& metaFile) = 0;
};

template<class T>
class MetaFileDrawer : public MetaFileDrawerBase
{
public:
    MetaFileDrawer() {}
    ~MetaFileDrawer() {}
    bool Draw(MetaFile& metaFile) override
    {
        T& derived = static_cast<T&>(metaFile);
        return DrawImpl(derived);
    }
    virtual bool DrawImpl(T& metaFile) = 0;
};
