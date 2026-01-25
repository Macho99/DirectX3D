#include "pch.h"
#include "Inspector.h"

Inspector::Inspector()
{
}

Inspector::~Inspector()
{
}

void Inspector::OnGUI()
{
    Super::OnGUI();

    ImGui::Begin("Inspector", &IsOpen);
    ImGui::Text("Inspector Content");
    ImGui::End();
}
