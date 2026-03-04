#pragma once
#include "ResourceDrawer.h"
#include "Material.h"

class MaterialDrawer : public ResourceDrawer<Material>
{
public:
    MaterialDrawer();
    ~MaterialDrawer();

    virtual bool DrawImpl(Material& resource) override;
};

