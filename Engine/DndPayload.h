#pragma once

namespace DndPayload
{
    inline constexpr const char* Entity = "DND_ENTITY";
    inline constexpr const char* Asset = "DND_ASSET";
    inline constexpr const char* FilePath = "DND_FILEPATH";

    void GameObjectSource(const GameObjectRef& objRef, function<void()> onDrag = nullptr);
    bool GameObjectTarget(OUT GameObjectRef& outDroppedRef, function<void()> onDrag = nullptr);

    template<class T>
    bool ComponentTarget(OUT ComponentRef<T>& outDroppedRef, function<void()> onDrag = nullptr);

    // Source: AssetId를 payload로 올림
    template<typename TDrawPreview>
    inline void AssetSource(const AssetId& assetId, TDrawPreview&& drawPreview)
    {
        if (!ImGui::BeginDragDropSource())
            return;

        // payload는 "복사"됩니다. (ImGui가 내부 저장)
        ImGui::SetDragDropPayload(DndPayload::Asset, &assetId, sizeof(AssetId));

        // 사용자가 원하는 미리보기 렌더링
        drawPreview();

        ImGui::EndDragDropSource();
    }

    // Target: 드랍되면 droppedId를 out에 채우고 true
    inline bool AssetTarget(OUT AssetId& outDroppedId)
    {
        if (!ImGui::BeginDragDropTarget())
            return false;

        bool accepted = false;

        // 선택: 미리 필터링(호버 상태에서만 반응하게)
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload && payload->IsDataType(DndPayload::Asset))
        {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(DndPayload::Asset))
            {
                outDroppedId = *reinterpret_cast<const AssetId*>(p->Data);
                accepted = true;
            }
        }

        ImGui::EndDragDropTarget();
        return accepted;
    }

    template<typename T>
    inline bool ResourceTarget(OUT ResourceRef<T>& outDroppedRef)
    {
        if (!ImGui::BeginDragDropTarget())
            return false;

        bool accepted = false;

        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload && payload->IsDataType(DndPayload::Asset))
        {
            const AssetId droppedId = *reinterpret_cast<const AssetId*>(payload->Data);
            if (RESOURCES->TryGetResourceRefByAssetId(droppedId, OUT outDroppedRef) == false)
                accepted = false;
            else if (ImGui::AcceptDragDropPayload(DndPayload::Asset))
            {
                accepted = true;
            }
        }

        ImGui::EndDragDropTarget();
        return accepted;
    }

    template<class T>
    bool ComponentTarget(ComponentRef<T>& outDroppedRef, function<void()> onDrag)
    {
        bool accepted = false;
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if (payload && payload->IsDataType(Entity))
            {
                const GameObjectRef droppedObjRef = *reinterpret_cast<const GameObjectRef*>(payload->Data);
                GameObject* droppedObj = droppedObjRef.Resolve();
                if (droppedObj != nullptr)
                {
                    ComponentRef<T> droppedCompRef(droppedObj->GetComponent<T>());
                    if (droppedCompRef.Resolve() != nullptr)
                    {
                        if (onDrag)
                            onDrag();

                        // Commit on drop
                        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(Entity))
                        {
                            outDroppedRef = droppedCompRef;
                            accepted = true;
                        }
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        return accepted;
    }
}
