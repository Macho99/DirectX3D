#pragma once
#include "Component.h"
#include "ComponentRef.h"

class Material;
class Mesh;
class MeshRenderer;
class Texture;

class UIImage : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(UIImage, Component)
public:
    UIImage();
    ~UIImage();

    virtual void Awake() override;
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    void SetTexture(ResourceRef<Texture> texture);
    ResourceRef<Texture> GetTexture() const { return _texture; }

    void SetMaterial(ResourceRef<Material> material);
    ResourceRef<Material> GetMaterial() const { return _material; }

    void SetColor(const Color& color);
    const Color& GetColor() const { return _color; }

    void SetPreserveAspect(bool preserveAspect);
    bool GetPreserveAspect() const { return _preserveAspect; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_texture));
        ar(CEREAL_NVP(_material));
        ar(CEREAL_NVP(_color));

        if (Archive::is_saving::value)
        {
            ar(CEREAL_NVP(_preserveAspect));
        }

        if (Archive::is_loading::value)
            _isDirty = true;
    }

private:
    void EnsureMeshRenderer();
    void RebuildMesh();
    void ApplyMaterial();
    Vec2 GetCurrentSize() const;

private:
    ResourceRef<Texture> _texture;
    ResourceRef<Material> _material;
    Color _color = Colors::White;
    bool _preserveAspect = false;
    bool _isDirty = false;
    Vec2 _lastSize = Vec2(0.0f, 0.0f);
    ComponentRef<MeshRenderer> _meshRenderer;
    ResourceRef<Mesh> _mesh;
};
