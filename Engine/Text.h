#pragma once
#include "UIRenderer.h"

class Font;
class Mesh;

#define TEXT_HORIZONTAL_START_LIST(X) \
    X(Left)                            \
    X(Center)                          \
    X(Right)

enum class TextHorizontalStart
{
#define X(name) name,
    TEXT_HORIZONTAL_START_LIST(X)
#undef X

    Max
};

static const char* TextHorizontalStartNames[] =
{
#define X(name) #name,
    TEXT_HORIZONTAL_START_LIST(X)
#undef X
};

#define TEXT_VERTICAL_START_LIST(X) \
    X(Top)                          \
    X(Middle)                       \
    X(Bottom)

enum class TextVerticalStart
{
#define X(name) name,
    TEXT_VERTICAL_START_LIST(X)
#undef X

    Max
};

static const char* TextVerticalStartNames[] =
{
#define X(name) #name,
    TEXT_VERTICAL_START_LIST(X)
#undef X
};

class Text : public UIRenderer
{
    using Super = UIRenderer;
    DECLARE_COMPONENT(Text)
    using TextString = string;
public:
    Text();
    ~Text();
    
    virtual void Awake() override;
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;
    virtual int GetVersion() const override { return 1; }

    void SetText(const TextString& text);
    const TextString& GetText() const { return _text; }
    void SetFont(ResourceRef<Font> font);
    ResourceRef<Font> GetFont() const { return _font; }
    void SetFontSize(float fontSize);
    float GetFontSize() const { return _fontSize; }
    void SetColor(const Color& color);
    const Color& GetColor() const { return _color; }
    void SetHorizontalStart(TextHorizontalStart horizontalStart);
    TextHorizontalStart GetHorizontalStart() const { return _horizontalStart; }
    void SetVerticalStart(TextVerticalStart verticalStart);
    TextVerticalStart GetVerticalStart() const { return _verticalStart; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_text));
        ar(CEREAL_NVP(_font));
        ar(CEREAL_NVP(_fontSize));
        ar(CEREAL_NVP(_color));

        if (Archive::is_saving::value || _version >= 1)
        {
            ar(CEREAL_NVP(_horizontalStart));
            ar(CEREAL_NVP(_verticalStart));
        }

        if (Archive::is_loading::value)
            _isDirty = true;
    }

private:
    void RebuildMesh();
    Vec2 GetFontMetricScale(int lineHeight);
    static bool DecodeNextUtf8(const string& text, size_t& index, int& codepoint);

private:
    TextString _text;
    ResourceRef<Font> _font;
    float _fontSize = 32.0f;
    Color _color = Colors::White;
    TextHorizontalStart _horizontalStart = TextHorizontalStart::Left;
    TextVerticalStart _verticalStart = TextVerticalStart::Middle;
    bool _isDirty = false;
    Vec3 _lastWorldScale = Vec3(0.0f, 0.0f, 0.0f);
};
