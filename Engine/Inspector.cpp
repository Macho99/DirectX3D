#include "pch.h"
#include "Inspector.h"

Inspector::Inspector()
    :Super("Inspector")
{
}

Inspector::~Inspector()
{
}

void Inspector::OnGUI()
{
    Super::OnGUI();

    ImGui::Begin(_windowName.c_str(), &IsOpen);
    ImGui::Text("Inspector Content");
    ImGui::End();
}
