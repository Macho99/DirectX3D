#include "pch.h"
#include "OnGUIUtils.h"

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

    if (isReadOnly)
        ImGui::BeginDisabled();

    bool changed = ImGui::ColorEdit4(label, color, flags);

    if (isReadOnly)
        ImGui::EndDisabled();

    return changed;
}
