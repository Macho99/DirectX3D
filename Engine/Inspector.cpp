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

    ImGui::Text("Inspector Content");
}
