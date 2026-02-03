#include "pch.h"
#include "Console.h"

Console::Console()
    : Super("Console")
{
}

Console::~Console()
{
}

void Console::Init()
{
}

void Console::OnGUI()
{
    Super::OnGUI();

    ImGui::Begin(_windowName.c_str(), &IsOpen);
    ImGui::Text("Console Content");
    ImGui::End();
}
