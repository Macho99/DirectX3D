#include "pch.h"
#include "Console.h"

Console::Console()
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

    ImGui::Begin("Console", &IsOpen);
    ImGui::Text("Console Content");
    ImGui::End();
}
