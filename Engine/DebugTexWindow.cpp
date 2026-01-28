#include "pch.h"
#include "DebugTexWindow.h"

DebugTexWindow::DebugTexWindow(wstring windowName, shared_ptr<Texture> debugTexture)
    :_windowName(windowName), _debugTexture(debugTexture)
{
}

DebugTexWindow::~DebugTexWindow()
{
}

void DebugTexWindow::OnGUI()
{
    ImGui::Begin(string(_windowName.begin(), _windowName.end()).c_str(), &IsOpen);

    Vec2 size = _debugTexture->GetSize();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float availAspect = avail.x / avail.y;

    ImGui::Image((ImTextureID)_debugTexture->GetComPtr().Get(), avail);

    ImGui::End();
}
