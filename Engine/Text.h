#pragma once
#include "Component.h"

class Font;
class Mesh;

class Text : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(Text)

    using TextString = string;
public:
    Text();
    ~Text();
    
    virtual void Awake() override;
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    void SetText(const TextString& text);
    const TextString& GetText() const { return _text; }
    void SetFont(ResourceRef<Font> font);
    ResourceRef<Font> GetFont() const { return _font; }
    void SetFontSize(float fontSize);
    float GetFontSize() const { return _fontSize; }
    void SetColor(const Color& color);
    const Color& GetColor() const { return _color; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_text));
        ar(CEREAL_NVP(_font));
        ar(CEREAL_NVP(_fontSize));
        ar(CEREAL_NVP(_color));

        if (Archive::is_loading::value)
            _isDirty = true;
    }

private:
    void EnsureMeshRenderer();
    void RebuildMesh();
    static bool DecodeNextUtf8(const string& text, size_t& index, int& codepoint);

private:
    TextString _text;
    ResourceRef<Font> _font;
    float _fontSize = 1.0f;
    Color _color = Colors::White;
    bool _isDirty = false;
    ComponentRef<class MeshRenderer> _meshRenderer;
    ResourceRef<Mesh> _mesh;
};