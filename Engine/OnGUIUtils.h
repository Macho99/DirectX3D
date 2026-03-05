#pragma once
#include "EditorManager.h"

class OnGUIUtils
{
public:
    static void DrawColor(const char* label, float* color, bool isReadOnly);

    template<class T>
    static void DrawResourceRef(const char* label, const ResourceRef<T>& resourceRef, bool isReadOnly);
};

template<class T>
inline void OnGUIUtils::DrawResourceRef(const char* label, const ResourceRef<T>& resourceRef, bool isReadOnly)
{
    bool allowClear = true;
    bool changed = false;

    ImGui::PushID(label);

    // 좌측 라벨
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();

    // 표시 문자열 만들기
    AssetId assetId = resourceRef.GetAssetId();
    const bool hasRef = assetId.IsValid();

    std::string display;
    if (hasRef)
    {
        MetaFile* meta = nullptr;
        if (RESOURCES->TryGetMetaByAssetId(assetId, OUT meta))
        {
            display = meta->GetName(assetId);
        }
    }

    if (display.empty())
    {
        display = hasRef ? "Missing (" + assetId.ToString() + ")" : "None";
    }

    // 필드 폭: X 버튼 공간 고려
    float fullW = ImGui::CalcItemWidth();
    float clearBtnW = allowClear ? (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x) : 0.0f;
    float fieldW = (fullW > clearBtnW) ? (fullW - clearBtnW) : fullW;

    // “필드” (버튼처럼 보이는 입력칸)
    ImGui::SetNextItemWidth(fieldW);
    bool clicked = ImGui::Button(display.c_str(), ImVec2(fieldW, 0));

    // 클릭 시 포커스
    if (clicked && hasRef)
    {
        EDITOR->FocusContentBrowserAsset(assetId);
    }

    // 드래그 드랍 수용
    //if (ImGui::BeginDragDropTarget())
    //{
    //    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dndPayloadType))
    //    {
    //        if (payload->DataSize == sizeof(AssetId128))
    //        {
    //            AssetId128 incoming = *(const AssetId128*)payload->Data;
    //            if (!(incoming == value))
    //            {
    //                value = incoming;
    //                changed = true;
    //            }
    //        }
    //    }
    //    ImGui::EndDragDropTarget();
    //}

    // 우클릭 메뉴(선택): Clear / Copy
    //if (ImGui::BeginPopupContextItem("RefFieldContext"))
    //{
    //    if (ImGui::MenuItem("Copy Id", nullptr, false, hasRef))
    //    {
    //        char tmp[40] = {};
    //        FormatAssetId(tmp, sizeof(tmp), value);
    //        ImGui::SetClipboardText(tmp);
    //    }
    //    if (ImGui::MenuItem("Clear", nullptr, false, hasRef))
    //    {
    //        value = {};
    //        changed = true;
    //    }
    //    ImGui::EndPopup();
    //}

    // X 버튼으로 Clear
    //if (allowClear)
    //{
    //    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    //    ImGui::BeginDisabled(!hasRef);
    //    if (ImGui::Button("X"))
    //    {
    //        value = {};
    //        changed = true;
    //    }
    //    ImGui::EndDisabled();
    //}

    ImGui::PopID();
}