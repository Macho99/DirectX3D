#include "pch.h"
#include "EditorWindow.h"

EditorWindow::EditorWindow(string windowName, bool enableMenu)
    : _windowName(windowName), _enableMenu(enableMenu)
{
}

void EditorWindow::CheckAndOnGUI()
{
    if (IsOpen == false)
        return;

    OnGUI();
}
