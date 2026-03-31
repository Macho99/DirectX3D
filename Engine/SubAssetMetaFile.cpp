#include "pch.h"
#include "SubAssetMetaFile.h"
#include "DndPayload.h"
#include "EditorManager.h"

void SubAssetMetaFile::OnLoad(unordered_map<AssetId,MetaFile*, AssetIdHash>& subAssetContainer)
{
    Super::OnLoad(subAssetContainer);
    for (const SubAssetInfo& sub : _subAssets)
    {
        subAssetContainer[sub.assetId] = this;
    }
}

void SubAssetMetaFile::OnDestroy(unordered_map<AssetId,MetaFile*, AssetIdHash>& subAssetContainer)
{
    Super::OnDestroy(subAssetContainer);
    for (const SubAssetInfo& sub : _subAssets)
    {
        auto it = subAssetContainer.find(sub.assetId);
        ASSERT(it != subAssetContainer.end(), "SubAssetMetaFile::OnDestroy: sub asset not found in container: " + sub.assetId.ToString());
        ASSERT(it->second == this, "SubAssetMetaFile::OnDestroy: sub asset container mismatch for assetId: " + sub.assetId.ToString());
        subAssetContainer.erase(it);
    }
}

fs::path SubAssetMetaFile::GetImportedAssetPath(const AssetId& assetId) const
{
    for (int i = 0; i < _subAssets.size(); i++)
    {
        if (_subAssets[i].assetId == assetId)
            return GetSubResourcePath(i);
    }
    if(assetId == _assetId)
    {
        ASSERT(false, "SubAssetMetaFile has no single resource path");
        return L"";
    }
    ASSERT(false, "SubAssetMetaFile::GetImportedAssetPath: assetId not found in subAssets: " + assetId.ToString());
    return L"";
}

wstring SubAssetMetaFile::GetSubResourcePath(int index) const
{
    if (index < 0 || index >= (int)_subAssets.size())
    {
        DBG->LogErrorW(L"[SubAssetMetaFile] GetResourcePath: index out of range: " + std::to_wstring(index));
        return L"";
    }
    return GetArtifactPath() + L"\\" + _subAssets[index].fileName;
}

void SubAssetMetaFile::DrawContentBrowserItem(fs::path& currentFolder, float thumbSize, int& curCol, int columns) const
{
    Super::DrawContentBrowserItem(currentFolder, thumbSize, curCol, columns);

    AssetRef selectedAsset;
    int selectedSubAssetIndex;
    EDITOR->TryGetContentBrowserAsset(OUT selectedAsset, OUT selectedSubAssetIndex);

    if (selectedAsset.GetAssetId() != _assetId)
        return;

    for (int i = 0; i < _subAssets.size(); i++)
    {
        const SubAssetInfo& sub = _subAssets[i];
        ImGui::PushID(sub.assetId.ToString().c_str());

        const float spacing = i == 0 ? 8.0f : 0.0f;
        if (curCol > 0) 
            ImGui::SameLine(0, spacing);

        // 1. 타일 레이아웃 설정
        const float tileW = thumbSize;
        const float lineH = ImGui::GetTextLineHeight();
        const float gapY = 4.0f;
        const float textH = lineH * 2.0f; // 딱 2줄 높이
        const float tileH = thumbSize + gapY + textH;

        ImGui::BeginGroup();
        ImVec2 tilePos = ImGui::GetCursorScreenPos();

        ImGui::SetCursorScreenPos(tilePos);
        // 2. 상호작용 영역 (클릭/호버 감지용 투명 버튼)
        ImGui::InvisibleButton("##tile", ImVec2(tileW, tileH));
        bool hovered = ImGui::IsItemHovered();
        if (ImGui::BeginPopupContextItem("Context"))
        {
            if (ImGui::MenuItem("Copy"))
            {
                ImGui::SetClipboardText(sub.assetId.ToString().c_str());
            }
            ImGui::EndPopup();
        }

        bool selected = (selectedSubAssetIndex == i);

        ImTextureID iconTex = (ImTextureID)GetIconTexture(sub.resourceType, sub.assetId, GetSubResourcePath(i))->GetComPtr().Get();
        {
            // source
            DndPayload::AssetSource(sub.assetId, [&]
                {
                    ImGui::Image(iconTex, ImVec2(thumbSize * 0.9f, thumbSize * 0.9f));
                    ImGui::Text("Move: %s", Utils::ToString(sub.fileName).c_str());
                });

            // target
            AssetId dropped;
            if (DndPayload::AssetTarget(dropped))
            {
                DBG->LogW(L"Dropped asset " + dropped.ToWString() + L"=>" + sub.assetId.ToWString());
            }
        }

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            EDITOR->ClickAsset(sub.assetId);
        }

        // 3. 배경 그리기 (선택/호버)
        ImU32 bgCol = 0;
        if (hovered || selected) bgCol = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
        else bgCol = ImGui::GetColorU32(ImGuiCol_Header);

        if (bgCol != 0)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(tilePos, ImVec2(tilePos.x + tileW, tilePos.y + tileH), bgCol, 4.0f);
        }

        // 4. 아이콘 그리기
        ImGui::GetWindowDrawList()->AddImage(iconTex, tilePos, ImVec2(tilePos.x + thumbSize, tilePos.y + thumbSize));

        // 5. 텍스트 그리기 (2줄 제한 핵심 로직)
        ImVec2 textPos = ImVec2(tilePos.x, tilePos.y + thumbSize + gapY);
        ImVec2 textMax = ImVec2(textPos.x + tileW, textPos.y + textH);

        // 텍스트 출력 위치 강제 설정
        ImGui::SetCursorScreenPos(textPos);

        // 가로 폭 제한 (자동 줄바꿈 활성화)
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + tileW);

        // 세로 높이 제한 (3줄 이상은 그리지 않음)
        ImGui::GetWindowDrawList()->PushClipRect(textPos, textMax, true);

        // 텍스트 출력
        ImGui::TextWrapped("%s", Utils::ToString(sub.fileName).c_str());

        ImGui::GetWindowDrawList()->PopClipRect();
        ImGui::PopTextWrapPos();

        ImGui::EndGroup();
        ImGui::PopID();

        if (++curCol >= columns)
            curCol = 0;

        if (hovered)
        {
            ImGui::SetTooltip("%s\n%s", Utils::ToString(sub.fileName).c_str(), sub.assetId.ToString().c_str());
        }
    }
}

unique_ptr<ResourceBase> SubAssetMetaFile::LoadResource(AssetId assetId) const
{
    //Super::LoadResource(assetId);
    for (const SubAssetInfo & sub : _subAssets)
    {
        if (sub.assetId == assetId)
        {
            return LoadResource(sub.resourceType, fs::path(GetArtifactPath()) / sub.fileName);
        }
    }
    ASSERT(false, "SubAssetMetaFile::LoadResource: assetId not found: " + assetId.ToString());
    return nullptr;
}

AssetId SubAssetMetaFile::GetSubAssetIdByIndex(int subAssetIdx) const
{
    if (subAssetIdx < 0 || subAssetIdx >= (int)_subAssets.size())
    {
        DBG->LogErrorW(L"[SubAssetMetaFile] GetSubAssetIdByIndex: index out of range: " + std::to_wstring(subAssetIdx));
        return AssetId();
    }
    return _subAssets[subAssetIdx].assetId;
}

int SubAssetMetaFile::GetSubAssetIndexById(const AssetId& assetId) const
{
    for (int i = 0; i < _subAssets.size(); i++)
    {
        if (_subAssets[i].assetId == assetId)
            return i;
    }
    return -1;
}

string SubAssetMetaFile::GetName(const AssetId& assetId)
{
    if (assetId == _assetId)
        return Super::GetName(assetId);

    for (const SubAssetInfo& sub : _subAssets)
    {
        if (sub.assetId == assetId)
            return Super::GetName(assetId) + "/" + Utils::ToString(sub.fileName);
    }
    ASSERT(false, "SubAssetMetaFile::GetName: assetId not found: " + assetId.ToString());
    return "";
}

bool SubAssetMetaFile::TryGetSubAssetByType(ResourceType resourceType, OUT AssetId& assetId) const
{
    for (const SubAssetInfo& subAsset : _subAssets)
    {
        if (subAsset.resourceType == resourceType)
        {
            assetId = subAsset.assetId;
            return true;
        }
    }
    return false;
}
