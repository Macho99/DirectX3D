#pragma once
#include "Renderer.h"
class InstancingRenderer : public Renderer
{
public:
    using Super = Renderer;
    InstancingRenderer(ComponentType componentType);

    virtual AssetId GetMeshId() const = 0;
};

