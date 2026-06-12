#include "pch.h"
#include "Text.h"
#include "Font.h"
#include "GameObject.h"
#include "Geometry.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshRenderer.h"
#include "OnGUIUtils.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Utils.h"
#include "VertexData.h"
#include "RectTransform.h"

Text::Text() : Super(StaticType)
{
}

Text::~Text()
{
}

void Text::Awake()
{
    EnsureMeshRenderer();
    _isDirty = true;
}

void Text::Start()
{
    RebuildMesh();
}

void Text::Update()
{
    if (_isDirty)
    {
        RebuildMesh();

        MeshRenderer* meshRenderer = _meshRenderer.Resolve();
        if (meshRenderer != nullptr)
        {
            Material* material = meshRenderer->GetMaterial().Resolve();
            if (material != nullptr)
            {
                material->GetMaterialDesc().diffuse = _color;
            }
        }
    }
}

bool Text::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();

    char buffer[1024] = {};
    strncpy_s(buffer, _text.c_str(), _TRUNCATE);

    ImGui::PushID("Text");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Text");
    ImGui::SameLine();
    ImGui::SetCursorPosX(200.0f);
    if (ImGui::InputText("##value", buffer, IM_ARRAYSIZE(buffer)))
    {
        SetText(buffer);
        changed = true;
    }
    ImGui::PopID();

    bool colorChanged = OnGUIUtils::DrawColor("Color", &_color);
    changed |= colorChanged;
    if (colorChanged)
    {
        SetColor(_color);
    }

    if (OnGUIUtils::DrawResourceRef("Font", _font))
    {
        _isDirty = true;
        changed = true;
    }

    if (OnGUIUtils::DrawFloat("Font Size", &_fontSize, 0.01f))
    {
        _fontSize = std::max(0.0f, _fontSize);
        _isDirty = true;
        changed = true;
    }

    if (OnGUIUtils::DrawEnumCombo("Horizontal Start", _horizontalStart, TextHorizontalStartNames, (int)TextHorizontalStart::Max))
    {
        _isDirty = true;
        changed = true;
    }

    if (OnGUIUtils::DrawEnumCombo("Vertical Start", _verticalStart, TextVerticalStartNames, (int)TextVerticalStart::Max))
    {
        _isDirty = true;
        changed = true;
    }

    return changed;
}

void Text::SetText(const TextString& text)
{
    if (_text != text)
    {
        _text = text;
        _isDirty = true;
    }
}

void Text::SetFont(ResourceRef<Font> font)
{
    _font = font;
    _isDirty = true;
}

void Text::SetFontSize(float fontSize)
{
    fontSize = std::max(0.0f, fontSize);
    if (_fontSize != fontSize)
    {
        _fontSize = fontSize;
        _isDirty = true;
    }
}

void Text::SetColor(const Color& color)
{
    _color = color;
    MeshRenderer* meshRenderer = _meshRenderer.Resolve();
    if (meshRenderer != nullptr)
    {
        Material* material = meshRenderer->GetMaterial().Resolve();
        if (material != nullptr)
        {
            material->GetMaterialDesc().diffuse = _color;
        }
    }
}

void Text::SetHorizontalStart(TextHorizontalStart horizontalStart)
{
    if (_horizontalStart != horizontalStart)
    {
        _horizontalStart = horizontalStart;
        _isDirty = true;
    }
}

void Text::SetVerticalStart(TextVerticalStart verticalStart)
{
    if (_verticalStart != verticalStart)
    {
        _verticalStart = verticalStart;
        _isDirty = true;
    }
}

void Text::EnsureMeshRenderer()
{
    GameObject* gameObject = GetGameObject();
    if (gameObject == nullptr)
        return;

    MeshRenderer* meshRenderer = gameObject->GetMeshRenderer();
    if (meshRenderer == nullptr)
    {
        gameObject->AddComponent(make_unique<MeshRenderer>());
        meshRenderer = gameObject->GetMeshRenderer();
    }

    _meshRenderer = gameObject->GetFixedComponentRef<MeshRenderer>();

    if (_mesh.IsValid() == false)
        _mesh = RESOURCES->AllocateTempResource<Mesh>();

    if (meshRenderer != nullptr)
        meshRenderer->SetMesh(_mesh);
}

void Text::RebuildMesh()
{
    _isDirty = false;
    EnsureMeshRenderer();

    Font* font = _font.Resolve();
    Mesh* mesh = _mesh.Resolve();
    if (font == nullptr || mesh == nullptr)
        return;

    const int lineHeight = font->GetLineHeight();
    const float scale = lineHeight > 0 ? _fontSize / static_cast<float>(lineHeight) : 1.0f;
    auto geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();

    vector<float> lineWidths(1, 0.0f);
    {
        float lineWidth = 0.0f;
        int previousCodepointForMeasure = 0;
        size_t measureIndex = 0;
        while (measureIndex < _text.size())
        {
            int codepoint = 0;
            if (DecodeNextUtf8(_text, measureIndex, codepoint) == false)
                continue;

            if (codepoint == '\r')
                continue;

            if (codepoint == '\n')
            {
                lineWidth = 0.0f;
                previousCodepointForMeasure = 0;
                lineWidths.push_back(0.0f);
                continue;
            }

            const BMFontGlyph* glyph = font->GetGlyph(codepoint);
            if (glyph == nullptr)
                continue;

            lineWidth += static_cast<float>(font->GetKerning(previousCodepointForMeasure, codepoint)) * scale;
            lineWidth += static_cast<float>(glyph->xAdvance) * scale;
            lineWidths.back() = lineWidth;
            previousCodepointForMeasure = codepoint;
        }
    }

    float horizontalAnchorX = 0.0f;
    float initialCursorY = 0.0f;
    RectTransform* rectTransform = dynamic_cast<RectTransform*>(GetTransform());
    if (rectTransform != nullptr)
    {
        switch (_horizontalStart)
        {
        case TextHorizontalStart::Left:
            horizontalAnchorX = -0.5f;
            break;
        case TextHorizontalStart::Center:
            horizontalAnchorX = 0.0f;
            break;
        case TextHorizontalStart::Right:
            horizontalAnchorX = 0.5f;
            break;
        }

        switch (_verticalStart)
        {
        case TextVerticalStart::Top:
            initialCursorY = 0.5f;
            break;
        case TextVerticalStart::Middle:
            initialCursorY = 0.0f;
            initialCursorY += static_cast<float>(lineHeight) * scale * 0.5f;
            break;
        case TextVerticalStart::Bottom:
            initialCursorY = -0.5f;
            initialCursorY += static_cast<float>(lineHeight) * scale;
            break;
        }
    }

    auto getLineStartX = [&](size_t lineIndex) -> float
    {
        const float lineWidth = lineIndex < lineWidths.size() ? lineWidths[lineIndex] : 0.0f;
        switch (_horizontalStart)
        {
        case TextHorizontalStart::Center:
            return horizontalAnchorX - lineWidth * 0.5f;
        case TextHorizontalStart::Right:
            return horizontalAnchorX - lineWidth;
        case TextHorizontalStart::Left:
        default:
            return horizontalAnchorX;
        }
    };

    size_t lineIndex = 0;
    float initialCursorX = getLineStartX(lineIndex);
    float cursorX = initialCursorX;
    float cursorY = initialCursorY;
    int previousCodepoint = 0;


    size_t index = 0;
    while (index < _text.size())
    {
        int codepoint = 0;
        if (DecodeNextUtf8(_text, index, codepoint) == false)
            continue;

        if (codepoint == '\r')
            continue;

        if (codepoint == '\n')
        {
            ++lineIndex;
            initialCursorX = getLineStartX(lineIndex);
            cursorX = initialCursorX;
            cursorY -= static_cast<float>(lineHeight) * scale;
            previousCodepoint = 0;
            continue;
        }

        const BMFontGlyph* glyph = font->GetGlyph(codepoint);
        if (glyph == nullptr)
            continue;

        cursorX += static_cast<float>(font->GetKerning(previousCodepoint, codepoint)) * scale;

        const float left = cursorX + static_cast<float>(glyph->xOffset) * scale;
        const float top = cursorY - static_cast<float>(glyph->yOffset) * scale;
        const float right = left + static_cast<float>(glyph->width) * scale;
        const float bottom = top - static_cast<float>(glyph->height) * scale;

        const uint32 baseIndex = geometry->GetVertexCount();
        const Vec3 normal = Vec3(0.0f, 0.0f, -1.0f);
        const Vec3 tangent = Vec3(1.0f, 0.0f, 0.0f);

        geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(left, bottom, 0.0f), Vec2(glyph->u0, glyph->v1), normal, tangent });
        geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(left, top, 0.0f), Vec2(glyph->u0, glyph->v0), normal, tangent });
        geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(right, bottom, 0.0f), Vec2(glyph->u1, glyph->v1), normal, tangent });
        geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(right, top, 0.0f), Vec2(glyph->u1, glyph->v0), normal, tangent });

        geometry->AddIndices({ baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 1, baseIndex + 3 });

        cursorX += static_cast<float>(glyph->xAdvance) * scale;
        previousCodepoint = codepoint;
    }

    mesh->CreateFromGeometry(geometry);
}

bool Text::DecodeNextUtf8(const string& text, size_t& index, int& codepoint)
{
    if (index >= text.size())
        return false;

    const unsigned char c0 = static_cast<unsigned char>(text[index++]);
    if (c0 < 0x80)
    {
        codepoint = c0;
        return true;
    }

    int additionalBytes = 0;
    codepoint = 0;

    if ((c0 & 0xE0) == 0xC0)
    {
        additionalBytes = 1;
        codepoint = c0 & 0x1F;
    }
    else if ((c0 & 0xF0) == 0xE0)
    {
        additionalBytes = 2;
        codepoint = c0 & 0x0F;
    }
    else if ((c0 & 0xF8) == 0xF0)
    {
        additionalBytes = 3;
        codepoint = c0 & 0x07;
    }
    else
    {
        return false;
    }

    for (int i = 0; i < additionalBytes; ++i)
    {
        if (index >= text.size())
            return false;

        const unsigned char cx = static_cast<unsigned char>(text[index++]);
        if ((cx & 0xC0) != 0x80)
            return false;

        codepoint = (codepoint << 6) | (cx & 0x3F);
    }

    return true;
}
