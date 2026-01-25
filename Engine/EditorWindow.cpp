#include "pch.h"
#include "EditorWindow.h"

void EditorWindow::CheckAndOnGUI()
{
    if (IsOpen == false)
        return;

    OnGUI();
}
