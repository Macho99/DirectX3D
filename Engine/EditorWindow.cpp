#include "pch.h"
#include "EditorWindow.h"

EditorWindow::EditorWindow(string windowName, bool enableMenu)
    : _windowName(windowName), _enableMenu(enableMenu)
{
}

void EditorWindow::Init(EditorManager* editorManager)
{
    _editorManager = editorManager;
}

void EditorWindow::CheckAndOnGUI()
{
    if (IsOpen == false)
        return;

    ImGui::Begin(_windowName.c_str(), &IsOpen);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::SetWindowFocus();
    }

    OnGUI();
    ImGui::End();
}
