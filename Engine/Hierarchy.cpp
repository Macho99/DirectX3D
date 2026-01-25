#include "pch.h"
#include "Hierarchy.h"

void Hierarchy::Init()
{
}

void Hierarchy::OnGUI()
{
    Super::OnGUI();

    ImGui::Begin("Hierarchy", &IsOpen);
    ImGui::Text("Hierarchy Content");
    ImGui::End();
}
