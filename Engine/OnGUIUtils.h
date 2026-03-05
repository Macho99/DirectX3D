#pragma once
#include "EditorManager.h"
#include "DndPayload.h"

class OnGUIUtils
{
public:
    static bool DrawColor(const char* label, float* color, bool isReadOnly);

    template<typename T>
    static bool DrawResourceRef(const char* label, ResourceRef<T>& resourceRef, bool isReadOnly);
};

template<typename T>
inline bool OnGUIUtils::DrawResourceRef(const char* label, ResourceRef<T>& resourceRef, bool isReadOnly)
{
    bool changed = false;
    const bool allowClear = true;

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

    if(isReadOnly == false)
    {
        ResourceRef<T> dropped;
        if (DndPayload::ResourceTarget(OUT dropped))
        {
            resourceRef = dropped;
            changed = true;
        }
    }

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
    return changed;
}