#include "pch.h"
#include "DebugTexWindow.h"

DebugTexWindow::DebugTexWindow(string windowName, function<Texture*()> getDebugTexture)
    : Super(windowName), _getDebugTexture(getDebugTexture)
{
    _windowName = windowName;
}

DebugTexWindow::~DebugTexWindow()
{
}

void DebugTexWindow::OnGUI()
{
    ImGui::Begin(_windowName.c_str(), &IsOpen); 

    Texture* debugTexture = _getDebugTexture();

    Vec2 size = debugTexture->GetSize();
    if (size.x < 10 || size.y < 10)
    {
        ImGui::Text("Texture Size not available");
        ImGui::End();
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float availAspect = avail.x / avail.y;
    float texAspect = size.x / size.y;

    ImVec2 imageSize = avail;
    if (availAspect > texAspect)
        imageSize.x = avail.y * texAspect;
    else
        imageSize.y = avail.x / texAspect;

    // ===== 중앙 정렬 =====
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImVec2 offset(
        (avail.x - imageSize.x) * 0.5f,
        (avail.y - imageSize.y) * 0.5f
    );
    ImGui::SetCursorPos(ImVec2(cursorPos.x + offset.x, cursorPos.y + offset.y));

    // ===== 이미지 출력 =====
    ImGui::Image(
        (ImTextureID)debugTexture->GetComPtr().Get(),
        imageSize
    );

    // ===== 테두리 그리기 =====
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(
        min,
        max,
        IM_COL32(255, 255, 255, 255), // 색상
        0.0f,                         // 라운드
        0,
        1.0f                          // 두께
    );

    ImGui::End();
}
