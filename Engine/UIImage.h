#pragma once
#include "UIRenderer.h"

class Material;
class Mesh;
class Texture;

class UIImage : public UIRenderer
{
    using Super = UIRenderer;
    DECLARE_COMPONENT(UIImage)
public:
    UIImage();
    ~UIImage();

    virtual void Awake() override;
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    void SetTexture(ResourceRef<Texture> texture);
    ResourceRef<Texture> GetTexture() const { return _texture; }

    virtual void SetMaterial(ResourceRef<Material> material) override;

    void SetColor(const Color& color);
    const Color& GetColor() const { return _color; }

    void SetPreserveAspect(bool preserveAspect);
    bool GetPreserveAspect() const { return _preserveAspect; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_texture));
        ar(CEREAL_NVP(_color));

        if (Archive::is_saving::value)
        {
            ar(CEREAL_NVP(_preserveAspect));
        }

        if (Archive::is_loading::value)
        {
            _isDirty = true;
            ApplyMaterial();
        }
    }

private:
    void RebuildMesh();
    void ApplyMaterial();
    Vec2 GetCurrentSize() const;

private:
    ResourceRef<Texture> _texture;
    Color _color = Colors::White;
    bool _preserveAspect = false;
    bool _isDirty = false;
    Vec2 _lastSize = Vec2(0.0f, 0.0f);
};
