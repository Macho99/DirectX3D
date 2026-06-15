#pragma once
#include "Renderer.h"

class Mesh;

class UIRenderer : public Renderer
{
    using Super = Renderer;
public:
    UIRenderer(ComponentType componentType);

    void SetMesh(ResourceRef<Mesh> mesh) { _mesh = mesh; }
    ResourceRef<Mesh> GetMesh() { return _mesh; }
    void SetMaskMode(UIMaskMode mode);
    UIMaskMode GetMaskMode() const { return _maskMode; }

    virtual void InnerRender(RenderTech renderTech) override;
    virtual bool OnGUI() override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

        if (Archive::is_saving::value)
            ar(CEREAL_NVP(_maskMode));
    }

protected:
    ResourceRef<Mesh> _mesh;
    UIMaskMode _maskMode = UIMaskMode::None;
};
