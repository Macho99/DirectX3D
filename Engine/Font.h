#pragma once
#include "ResourceBase.h"

struct BMFontGlyph
{
    int id = 0;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    int xOffset = 0;
    int yOffset = 0;
    int xAdvance = 0;

    int page = 0;
    int channel = 0;

    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
};

struct BMFontKerningKey
{
    int first = 0;
    int second = 0;

    bool operator==(const BMFontKerningKey& other) const
    {
        return first == other.first && second == other.second;
    }
};

struct BMFontKerningKeyHash
{
    std::size_t operator()(const BMFontKerningKey& key) const
    {
        return (static_cast<std::size_t>(key.first) << 32)
            ^ static_cast<std::size_t>(key.second);
    }
};

class Font : public ResourceBase
{
    using Super = ResourceBase;
public:
    static constexpr ResourceType StaticType = ResourceType::Font;
    Font();
    ~Font();

    void Load(const wstring& path) override;

    const BMFontGlyph* GetGlyph(int charCode) const;
    int GetKerning(int first, int second) const;

    int GetLineHeight() const { return _lineHeight; }
    int GetBase() const { return _base; }
    int GetAtlasWidth() const { return _atlasWidth; }
    int GetAtlasHeight() const { return _atlasHeight; }

    const string& GetTextureFileName() const { return _textureFileName; }

private:
    static unordered_map<string, string> ParseKeyValues(const std::string& line);
    static string Unquote(const string& value);

private:
    int _lineHeight = 0;
    int _base = 0;
    int _atlasWidth = 0;
    int _atlasHeight = 0;

    string _textureFileName;

    unordered_map<int, BMFontGlyph> _glyphs;
    unordered_map<BMFontKerningKey, int, BMFontKerningKeyHash> _kernings;
};

