#pragma once


namespace DndPayload
{
    inline constexpr const char* Entity = "DND_ENTITY";
    inline constexpr const char* Asset = "DND_ASSET";
    inline constexpr const char* FilePath = "DND_FILEPATH";

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
        AssetId droppedId;
        if (AssetTarget(droppedId) == false)
            return false;

        if (RESOURCES->TryGetResourceRefByAssetId(droppedId, OUT outDroppedRef) == false)
            return false;

        return true;
    }
}
