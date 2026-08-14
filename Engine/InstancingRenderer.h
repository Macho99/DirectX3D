#pragma once
#include "Renderer.h"
class InstancingRenderer : public Renderer
{
public:
    using Super = Renderer;
    InstancingRenderer(ComponentType componentType);

    virtual AssetId GetMeshId() const = 0;

protected:
    virtual void OnMaterialChange(const Material* oldMaterial, const Material* newMaterial) override;
    void OnMeshChange(const AssetId& oldMeshId, const AssetId& newMeshId);
};

