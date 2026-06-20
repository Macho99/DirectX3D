#pragma once
#include "InputManager.h"
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

    Vec2 GetMousePosition() const;
    Vec2 GetLocalMousePosition();
    bool ContainsMouseSelf();

    virtual void InnerRender(RenderTech renderTech) override;
    virtual bool OnGUI() override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

        if (Archive::is_saving::value)
            ar(CEREAL_NVP(_maskMode));
    }

public:
    virtual void OnMouseEnter() { }

    virtual void OnMouseStay() { }
    virtual void OnMouseDown() { }
    virtual void OnMouseDrag(Vec2 delta) { }
    virtual void OnMouseUp() { }

    virtual void OnMouseExit() { }

protected:
    ResourceRef<Mesh> _mesh;
    UIMaskMode _maskMode = UIMaskMode::None;
};
