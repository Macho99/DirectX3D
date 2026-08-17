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
    bool IsIgnoreMouseInput() const { return _ignoreMouseInput; }
    void SetIgnoreMouseInput(bool value) { _ignoreMouseInput = value; }

    virtual void InnerRender(RenderTech renderTech) override;
    virtual bool OnGUI() override;
    virtual int GetVersion() const { return Super::GetVersion() + 2; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

        if (Archive::is_saving::value)
            ar(CEREAL_NVP(_maskMode));

        if (_version >= 2)
        {
            ar(CEREAL_NVP(_ignoreMouseInput));
        }
    }

public:
    virtual void OnMouseEnter() { _containsMouseSelf = true; }

    virtual void OnMouseStay() { }
    virtual void OnMouseDown() { }
    virtual void OnMouseUp() { }

    virtual void OnMouseExit() { _containsMouseSelf = false; }

protected:
    ResourceRef<Mesh> _mesh;
    UIMaskMode _maskMode = UIMaskMode::None;
    bool _containsMouseSelf = false;
    bool _ignoreMouseInput = false;
};
