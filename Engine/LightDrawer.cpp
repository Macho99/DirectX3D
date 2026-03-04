#include "pch.h"
#include "LightDrawer.h"

bool LightDrawer::DrawImpl(Light& component)
{
    bool showAlpha = true; // 알파 채널 표시 여부

    ImGuiColorEditFlags flags = 0;
    flags |= ImGuiColorEditFlags_DisplayRGB;        // RGB 표시
    flags |= ImGuiColorEditFlags_InputRGB;          // RGB 입력
    flags |= ImGuiColorEditFlags_Float;             // 0~1 float
    //flags |= ImGuiColorEditFlags_HDR;
    if (!showAlpha)
        flags |= ImGuiColorEditFlags_NoAlpha;


    LightDesc& lightDesc = component.GetLightDesc();
    ImGui::ColorEdit4("Ambient", &lightDesc.ambient.x, flags);
    ImGui::ColorEdit4("Diffuse", &lightDesc.diffuse.x, flags);
    ImGui::ColorEdit4("Specular", &lightDesc.specular.x, flags);
    ImGui::ColorEdit4("Emissive", &lightDesc.emissive.x, flags);

    return false;
}
