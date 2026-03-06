#include "pch.h"
#include "OnGUIUtils.h"

const float OnGUIUtils::_labelWidth = 200.0f;
const char* const OnGUIUtils::_valueLabel = "##value";

bool OnGUIUtils::DrawBool(const char* label, bool* value, bool isReadOnly)
{
    Begin(label, isReadOnly);


    bool changed = ImGui::Checkbox(_valueLabel, value);

    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawUInt8(const char* label, uint8* value, float dragSpeed, bool isReadOnly)
{
    return DrawScalar(label, ImGuiDataType_U8, value, dragSpeed, isReadOnly);
}

bool OnGUIUtils::DrawUInt32(const char* label, uint32* value, float dragSpeed, bool isReadOnly)
{
    return DrawScalar(label, ImGuiDataType_U32, value, dragSpeed, isReadOnly);
}

bool OnGUIUtils::DrawInt32(const char* label, int* value, float dragSpeed, bool isReadOnly)
{
    return DrawScalar(label, ImGuiDataType_S32, value, dragSpeed, isReadOnly);
}

bool OnGUIUtils::DrawFloat(const char* label, float* value, float dragSpeed, bool isReadOnly)
{
    Begin(label, isReadOnly);
    bool changed = ImGui::DragFloat(_valueLabel, value, dragSpeed);
    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawVec3(const char* label, Vec3* value, float dragSpeed, bool isReadOnly)
{
    Begin(label, isReadOnly);
    bool changed = ImGui::DragFloat3(_valueLabel, &(*value).x, dragSpeed);
    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawColor(const char* label, float* color, bool isReadOnly)
{
    bool showAlpha = true;

    ImGuiColorEditFlags flags = 0;
    flags |= ImGuiColorEditFlags_DisplayRGB;        // RGB 표시
    flags |= ImGuiColorEditFlags_InputRGB;          // RGB 입력
    flags |= ImGuiColorEditFlags_Float;             // 0~1 float
    //flags |= ImGuiColorEditFlags_HDR;
    if (!showAlpha)
        flags |= ImGuiColorEditFlags_NoAlpha;

    Begin(label, isReadOnly);

    bool changed = ImGui::ColorEdit4(_valueLabel, color, flags);

    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawScalar(const char* label, ImGuiDataType dataType, void* value, float dragSpeed, bool isReadOnly)
{
    Begin(label, isReadOnly);
    bool changed = ImGui::DragScalar(_valueLabel, dataType, value, dragSpeed);
    End(isReadOnly);
    return changed;
}

void OnGUIUtils::Begin(const char* label, bool setDisable)
{
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(_labelWidth);
    //ImGui::PushItemWidth(-1);

    if (setDisable)
        ImGui::BeginDisabled();
}

void OnGUIUtils::End(bool setDisable)
{
    if (setDisable)
        ImGui::EndDisabled();

    //ImGui::PopItemWidth();
    ImGui::PopID();
}
