#include "pch.h"
#include "OnGUIUtils.h"

const float OnGUIUtils::_labelWidth = 200.0f;
const char* const OnGUIUtils::_valueLabel = "##value";

bool OnGUIUtils::DrawBool(const char* label, bool* value, bool isReadOnly)
{
    Begin(label, isReadOnly);


    bool changed = ImGui::Checkbox(_valueLabel, value);

    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawUInt8(const char* label, uint8* value, float dragSpeed, bool isReadOnly)
{
    return DrawScalar(label, ImGuiDataType_U8, value, dragSpeed, isReadOnly);
}

bool OnGUIUtils::DrawUInt32(const char* label, uint32* value, float dragSpeed, bool isReadOnly)
{
    return DrawScalar(label, ImGuiDataType_U32, value, dragSpeed, isReadOnly);
}

bool OnGUIUtils::DrawInt32(const char* label, int* value, float dragSpeed, bool isReadOnly)
{
    return DrawScalar(label, ImGuiDataType_S32, value, dragSpeed, isReadOnly);
}

bool OnGUIUtils::DrawFloat(const char* label, float* value, float dragSpeed, bool isReadOnly)
{
    Begin(label, isReadOnly);
    bool changed = ImGui::DragFloat(_valueLabel, value, dragSpeed);
    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawVec3(const char* label, Vec3* value, float dragSpeed, bool isReadOnly)
{
    Begin(label, isReadOnly);
    bool changed = ImGui::DragFloat3(_valueLabel, &(*value).x, dragSpeed);
    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawColor(const char* label, float* color, bool isReadOnly)
{
    bool showAlpha = true;

    ImGuiColorEditFlags flags = 0;
    flags |= ImGuiColorEditFlags_DisplayRGB;        // RGB 표시
    flags |= ImGuiColorEditFlags_InputRGB;          // RGB 입력
    flags |= ImGuiColorEditFlags_Float;             // 0~1 float
    //flags |= ImGuiColorEditFlags_HDR;
    if (!showAlpha)
        flags |= ImGuiColorEditFlags_NoAlpha;

    Begin(label, isReadOnly);

    bool changed = ImGui::ColorEdit4(_valueLabel, color, flags);

    End(isReadOnly);

    return changed;
}

bool OnGUIUtils::DrawAssetRef(const char* label, AssetRef& assetRef, bool isReadOnly)
{
    bool changed = false;
    AssetId assetId = assetRef.GetAssetId();
    const bool hasRef = assetId.IsValid();
    BeginAssetRef(label, assetRef.GetAssetId(), isReadOnly);

    if (isReadOnly == false)
    {
        AssetId droppedId;
        if (DndPayload::AssetTarget(OUT droppedId))
        {
            assetRef = AssetRef(droppedId);
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
        AssetRef pastedRef;
        bool canPaste = false;
        if (clipboadText != nullptr) // 유효한 AssetId 길이
        {
            if (AssetId::TryParse(clipboadText, OUT pastedId))
            {
                canPaste = true;
                pastedRef = AssetRef(pastedId);
            }
        }
        if (ImGui::MenuItem("Paste", nullptr, false, canPaste))
        {
            assetRef = pastedRef;
            changed = true;
        }

        if (ImGui::MenuItem("Clear", nullptr, false, hasRef))
        {
            assetRef = AssetRef();
            changed = true;
        }
        ImGui::EndPopup();
    }

    End(false);
    return changed;

}

bool OnGUIUtils::DrawGameObjectRef(const char* label, GameObjectRef& gameObjectRef, bool isReadOnly)
{
    bool changed = false;
    const bool hasRef = gameObjectRef.IsValid();
    Guid guid;
    GameObject* gameObject = nullptr;

    std::string display;
    if (hasRef)
    {
        gameObject = gameObjectRef.Resolve();
        if (gameObject != nullptr)
        {
            guid = gameObject->GetGuid();
            display = gameObject->GetName();
        }
    }

    if (display.empty())
    {
        display = hasRef ? "Missing (" + guid.ToString() + ")" : "None";
    }

    bool clicked = BeginRef(label, display, isReadOnly);

    if (clicked && gameObject != nullptr)
    {
        EDITOR->FocusHierarchyTransform(gameObject->GetFixedComponentRef<Transform>());
    }

    if (isReadOnly == false)
    {
        GameObjectRef dropped;
        if (DndPayload::GameObjectTarget(OUT dropped))
        {
            gameObjectRef = dropped;
            changed = true;
        }
    }

    // 우클릭 메뉴(선택): Clear / Copy
    if (ImGui::BeginPopupContextItem("RefFieldContext"))
    {
        if (ImGui::MenuItem("Copy", nullptr, false, hasRef))
        {
            ImGui::SetClipboardText(guid.ToString().c_str());
        }

        const char* clipboadText = ImGui::GetClipboardText();
        Guid pastedId;
        bool canPaste = false;
        GameObjectRef pastedRef;
        if (clipboadText != nullptr) // 유효한 AssetId 길이
        {
            if (Guid::TryParse(clipboadText, OUT pastedId))
            {
                pastedRef = GameObject::GetGameObjectRefByGuid(pastedId);
                if (pastedRef.IsValid())
                {
                    canPaste = true;
                }
            }
        }

        if (ImGui::MenuItem("Paste", nullptr, false, canPaste))
        {
            gameObjectRef = pastedRef;
            changed = true;
        }

        if (ImGui::MenuItem("Clear", nullptr, false, hasRef))
        {
            gameObjectRef = GameObjectRef();
            changed = true;
        }
        ImGui::EndPopup();
    }

    End(false);
    return changed;
}

bool OnGUIUtils::DrawScalar(const char* label, ImGuiDataType dataType, void* value, float dragSpeed, bool isReadOnly)
{
    Begin(label, isReadOnly);
    bool changed = ImGui::DragScalar(_valueLabel, dataType, value, dragSpeed);
    End(isReadOnly);
    return changed;
}

void OnGUIUtils::Begin(const char* label, bool setDisable)
{
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(_labelWidth);
    //ImGui::PushItemWidth(-1);

    if (setDisable)
        ImGui::BeginDisabled();
}

bool OnGUIUtils::BeginRef(const char* label, const string& display, bool setDisable)
{
    Begin(label, false);

    if (setDisable)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    // “필드” (버튼처럼 보이는 입력칸)
    //ImGui::SetNextItemWidth(fieldW);
    bool clicked = ImGui::Button(display.c_str(), ImVec2(ImGui::CalcItemWidth(), 0));

    if (setDisable)
        ImGui::PopStyleColor();

    return clicked;
}

void OnGUIUtils::BeginAssetRef(const char* label, const AssetId& assetId, bool setDisable)
{
    // 표시 문자열 만들기
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

    bool clicked = BeginRef(label, display, setDisable);

    // 클릭 시 포커스
    if (clicked && hasRef)
    {
        EDITOR->FocusContentBrowserAsset(assetId);
    }
}

void OnGUIUtils::End(bool setDisable)
{
    if (setDisable)
        ImGui::EndDisabled();

    //ImGui::PopItemWidth();
    ImGui::PopID();
}
