#include "pch.h"
#include "DndPayload.h"

void DndPayload::GameObjectSource(const GameObjectRef& objRef, function<void()> onDrag)
{
    GameObject* obj = objRef.Resolve();
    if (obj == nullptr)
    {
        DBG->LogError("DndPayload::GameObjectSource: Failed to resolve GameObjectRef.");
        return;
    }

    // Drag source
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload(Entity, &objRef, sizeof(GameObjectRef));
        ImGui::Text("Move: %s", obj->GetName().c_str());
        ImGui::EndDragDropSource();
    }
}

bool DndPayload::GameObjectTarget(OUT GameObjectRef& outDroppedRef, function<void()> onDrag)
{
    bool accepted = false;
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload && payload->IsDataType(Entity))
        {
            if (onDrag)
                onDrag();

            // Commit on drop
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(Entity))
            {
                outDroppedRef = *(const GameObjectRef*)p->Data;
                accepted = true;
            }
        }

        ImGui::EndDragDropTarget();
    }

    return accepted;
}
