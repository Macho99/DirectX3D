#pragma once
#include "EditorManager.h"
#include "DndPayload.h"

class OnGUIUtils
{
public:
    static bool DrawBool(const char* label, bool* value, bool isReadOnly = false);
    static bool DrawUInt8(const char* label, uint8* value, float dragSpeed, bool isReadOnly = false);
    static bool DrawUInt32(const char* label, uint32* value, float dragSpeed, bool isReadOnly = false);
    static bool DrawInt32(const char* label, int* value, float dragSpeed, bool isReadOnly = false);
    static bool DrawFloat(const char* label, float* value, float dragSpeed, bool isReadOnly = false);
    static bool DrawVec3(const char* label, Vec3* value, float dragSpeed, bool isReadOnly = false);
    static bool DrawColor(const char* label, float* color, bool isReadOnly = false);
    
    template<typename TEnum>
    static bool DrawEnumCombo(const char* label, TEnum& value, const char* const* names, int count, bool isReadOnly = false);

    template<typename T>
    static bool DrawResourceRef(const char* label, ResourceRef<T>& resourceRef, bool isReadOnly = false);

private:
    static bool DrawScalar(const char* label, ImGuiDataType dataType, void* value, float dragSpeed, bool isReadOnly = false);
    static void Begin(const char* label, bool setDisable);
    static void End(bool setDisable);

private:
    static const float _labelWidth;
    static const char* const _valueLabel;
};

template<typename TEnum>
inline bool OnGUIUtils::DrawEnumCombo(const char* label, TEnum& value, const char* const* names, int count, bool isReadOnly)
{
    int current = (int)value;
    bool changed = false;

    Begin(label, isReadOnly);

    if (ImGui::Combo(_valueLabel, &current, names, count))
    {
        value = (TEnum)current;
        changed = true;
    }

    End(isReadOnly);

    return changed;
}

template<typename T>
inline bool OnGUIUtils::DrawResourceRef(const char* label, ResourceRef<T>& resourceRef, bool isReadOnly)
{
    bool changed = false;

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

    Begin(label, false);

    if(isReadOnly)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    // “필드” (버튼처럼 보이는 입력칸)
    //ImGui::SetNextItemWidth(fieldW);
    bool clicked = ImGui::Button(display.c_str(), ImVec2(ImGui::CalcItemWidth(), 0));
    
    if(isReadOnly)
        ImGui::PopStyleColor();

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
    if (ImGui::BeginPopupContextItem("RefFieldContext"))
    {
        if (ImGui::MenuItem("Copy", nullptr, false, hasRef))
        {
            ImGui::SetClipboardText(assetId.ToString().c_str());
        }

        const char* clipboadText = ImGui::GetClipboardText();
        AssetId pastedId;
        bool canPaste = false;
        ResourceRef<T> pastedRef;
        if (clipboadText != nullptr) // 유효한 AssetId 길이
        {
            if (AssetId::TryParse(clipboadText, OUT pastedId))
            {
                if (RESOURCES->TryGetResourceRefByAssetId(pastedId, OUT pastedRef))
                {
                    canPaste = true;
                }
            }
        }
        if (ImGui::MenuItem("Paste", nullptr, false, canPaste))
        {
            resourceRef = pastedRef;
            changed = true;
        }

        if (ImGui::MenuItem("Clear", nullptr, false, hasRef))
        {
            resourceRef = ResourceRef<T>();
            changed = true;
        }
        ImGui::EndPopup();
    }

    End(false);
    return changed;
}