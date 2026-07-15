#include "pch.h"
#include "Font.h"
#include <fstream>

Font::Font() : Super(StaticType)
{
}

Font::~Font()
{
}

void Font::Load(const wstring& path)
{
    Super::Load(path);

    std::ifstream file(path);

    if (!file.is_open())
        return;

    _glyphs.clear();
    _kernings.clear();

    string line;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::istringstream iss(line);

        string type;
        iss >> type;

        const auto values = ParseKeyValues(line);

        if (type == "common")
        {
            if (values.contains("lineHeight"))
                _lineHeight = std::stoi(values.at("lineHeight"));

            if (values.contains("base"))
                _base = std::stoi(values.at("base"));

            if (values.contains("scaleW"))
                _atlasWidth = std::stoi(values.at("scaleW"));

            if (values.contains("scaleH"))
                _atlasHeight = std::stoi(values.at("scaleH"));
        }
        else if (type == "page")
        {
            if (values.contains("file"))
                _textureFileName = Unquote(values.at("file"));
        }
        else if (type == "char")
        {
            BMFontGlyph glyph;

            glyph.id = std::stoi(values.at("id"));
            glyph.x = std::stoi(values.at("x"));
            glyph.y = std::stoi(values.at("y"));
            glyph.width = std::stoi(values.at("width"));
            glyph.height = std::stoi(values.at("height"));
            glyph.xOffset = std::stoi(values.at("xoffset"));
            glyph.yOffset = std::stoi(values.at("yoffset"));
            glyph.xAdvance = std::stoi(values.at("xadvance"));

            if (values.contains("page"))
                glyph.page = std::stoi(values.at("page"));

            if (values.contains("chnl"))
                glyph.channel = std::stoi(values.at("chnl"));

            if (_atlasWidth > 0 && _atlasHeight > 0)
            {
                glyph.u0 = static_cast<float>(glyph.x) / static_cast<float>(_atlasWidth);
                glyph.v0 = static_cast<float>(glyph.y) / static_cast<float>(_atlasHeight);
                glyph.u1 = static_cast<float>(glyph.x + glyph.width) / static_cast<float>(_atlasWidth);
                glyph.v1 = static_cast<float>(glyph.y + glyph.height) / static_cast<float>(_atlasHeight);
            }

            _glyphs[glyph.id] = glyph;
        }
        else if (type == "kerning")
        {
            BMFontKerningKey key;
            key.first = std::stoi(values.at("first"));
            key.second = std::stoi(values.at("second"));

            const int amount = std::stoi(values.at("amount"));

            _kernings[key] = amount;
        }
    }
}

const BMFontGlyph* Font::GetGlyph(int charCode) const
{
    const auto it = _glyphs.find(charCode);

    if (it == _glyphs.end())
        return nullptr;

    return &it->second;
}

int Font::GetKerning(int first, int second) const
{
    const BMFontKerningKey key{ first, second };

    const auto it = _kernings.find(key);

    if (it == _kernings.end())
        return 0;

    return it->second;
}

unordered_map<string, string> Font::ParseKeyValues(const string& line)
{
    unordered_map<string, string> result;

    size_t i = 0;

    while (i < line.size())
    {
        while (i < line.size() && line[i] == ' ')
            ++i;

        const size_t keyStart = i;

        while (i < line.size() && line[i] != '=' && line[i] != ' ')
            ++i;

        if (i >= line.size() || line[i] != '=')
        {
            while (i < line.size() && line[i] != ' ')
                ++i;
            continue;
        }

        const std::string key = line.substr(keyStart, i - keyStart);

        ++i; // skip '='

        std::string value;

        if (i < line.size() && line[i] == '"')
        {
            const std::size_t valueStart = i;

            ++i;

            while (i < line.size() && line[i] != '"')
                ++i;

            if (i < line.size())
                ++i;

            value = line.substr(valueStart, i - valueStart);
        }
        else
        {
            const std::size_t valueStart = i;

            while (i < line.size() && line[i] != ' ')
                ++i;

            value = line.substr(valueStart, i - valueStart);
        }

        result[key] = value;
    }

    return result;
}

string Font::Unquote(const std::string& value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        return value.substr(1, value.size() - 2);

    return value;
}
