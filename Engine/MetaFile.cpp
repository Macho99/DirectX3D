#include "pch.h"
#include "MetaFile.h"
#include "SubAssetMetaFile.h"
#include "TextureMeta.h"
#include "ModelMeta.h"
#include "FolderMeta.h"
#include "NotSupportMeta.h"
#include "AnimationMeta.h"
#include "MaterialMeta.h"
#include "MeshMeta.h"

#include "fstream"
#include "MetaStore.h"
#include "DndPayload.h"

MetaFile::MetaFile()
    :_resourceType(ResourceType::None)
    , _importManifest()
    , _assetId()
{
}

MetaFile::MetaFile(ResourceType resourceType)
    :_resourceType(resourceType)
    , _importManifest()
    , _assetId()
{
}

MetaFile::~MetaFile()
{
}

wstring MetaFile::GetResourcePath() const
{
    return GetArtifactPath() + L"\\asset";
}

bool MetaFile::ImportIfDirty()
{
    bool imported = false;
    const wstring manifestPath = GetManifestPath();
    if (fs::exists(manifestPath))
    {
        try
        {
            std::ifstream is(manifestPath);
            cereal::JSONInputArchive manifestArchive(is);
            manifestArchive(_importManifest);
        }
        catch (const std::exception& e)
        {
            DBG->LogW(L"[MetaFile] ImportIfDirty: Failed to read manifest and will reimport: " + manifestPath + L", error: " + Utils::ToWString(e.what()));
            _importManifest = ImportManifest();
        }
    }

    bool isDirty = false;
    bool isManifestRefreshed = _importManifest.Refresh(_absPath, isDirty);

    if (isDirty)
    {
        Import();
        imported = true;
    }

    if (isManifestRefreshed)
    {
        std::ofstream manifestOs(manifestPath);
        cereal::JSONOutputArchive manifestArchive(manifestOs);
        manifestArchive(_importManifest);
    }
    return imported;
}

void MetaFile::ForceReImport()
{
    _importManifest = ImportManifest();
    wstring manifestPath = GetManifestPath();
    if (fs::exists(manifestPath))
        fs::remove(manifestPath);

    ImportIfDirty();
}

void MetaFile::Import()
{
    wstring artifactFolder = GetArtifactPath();
    if (fs::exists(artifactFolder))
        fs::remove_all(artifactFolder);

    fs::create_directories(artifactFolder);
}

wstring MetaFile::GetArtifactPath() const
{
    if (!_assetId.IsValid())
        assert(false && "MetaFile::GetArtifactPath: invalid guid");

    return L"..\\Artifact\\" + _assetId.ToWString();
}

Texture* MetaFile::GetIconTexture(ResourceType resourceType, const AssetId& assetId, const fs::path& absPath) const
{
    auto& editorResources = RESOURCES->GetEditorResources();
    Texture* returnValue = nullptr;
    if (resourceType == ResourceType::Texture)
    {
        const string assetIdStr = assetId.ToString();
        auto it = editorResources.find(assetIdStr);

        if (it == editorResources.end())
        {
            unique_ptr<Texture> texture = make_unique<Texture>();
            texture->Load(absPath);
            returnValue = texture.get();
            editorResources[assetIdStr] = std::move(texture);
            return returnValue;
        }
        else
        {
            return static_cast<Texture*>(it->second.get());
        }
    }

    string iconKey;
    switch (resourceType)
    {
    case ResourceType::Animation:
        iconKey = "AnimationIcon";
        break;
    case ResourceType::Model:
        iconKey = "ModelIcon";
        break;
    case ResourceType::Folder:
        iconKey = "FolderIcon";
        break;
    case ResourceType::Material:
        iconKey = "MaterialIcon";
        break;
    case ResourceType::Mesh:
        iconKey = "MeshIcon";
        break;
    default:
        iconKey = "DefaultFileIcon";
        break;
    }

    auto it = editorResources.find(iconKey);
    if (it == editorResources.end())
    {
        unique_ptr<Texture> texture = make_unique<Texture>();
        texture->Load(L"..\\EditorResource\\" + Utils::ToWString(iconKey) + L".png");
        Texture* texturePtr = texture.get();
        editorResources[iconKey] = std::move(texture);
        return texturePtr;
    }
    else
    {
        return static_cast<Texture*>(it->second.get());
    }
}

Texture* MetaFile::GetIconTexture() const
{
    return GetIconTexture(GetResourceType(), GetAssetId(), GetAbsPath());
}

void MetaFile::DrawContentBrowserItem(fs::path& selectedPath, fs::path& currentFolder, float thumbSize, int& curCol, int columns) const
{
    fs::path absPath = GetAbsPath();
    bool isFolder = (GetResourceType() == ResourceType::Folder);
    std::string name = Utils::ToUtf8(absPath.filename());

    ImGui::PushID(absPath.string().c_str());
    if (curCol > 0) ImGui::SameLine();

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
    bool selected = (selectedPath == absPath);

    // source
    ImTextureID iconTex = (ImTextureID)GetIconTexture()->GetComPtr().Get();
    DndPayload::AssetSource(_assetId, [&]
        {
            ImGui::Image(iconTex, ImVec2(thumbSize * 0.9f, thumbSize * 0.9f));
            ImGui::Text("Move: %s", name.c_str());
        });

    // target
    AssetId dropped;
    if (DndPayload::AssetTarget(dropped))
    {
        DBG->LogW(L"Dropped asset " + dropped.ToWString() + L"=>" + _assetId.ToWString());
    }

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        selectedPath = absPath;
    }
    if (isFolder && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        currentFolder = absPath;
        selectedPath.clear();
    }

    // 3. 배경 그리기 (선택/호버)
    ImU32 bgCol = 0;
    if (selected) bgCol = ImGui::GetColorU32(ImGuiCol_HeaderActive);
    else if (hovered) bgCol = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

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
    ImGui::TextWrapped("%s", name.c_str());

    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::PopTextWrapPos();

    ImGui::EndGroup();
    ImGui::PopID();

    if (++curCol >= columns) 
        curCol = 0;
    if (hovered)
    {
        ImGui::SetTooltip("%s\n%s", name.c_str(), _assetId.ToString().c_str());
    }
}

//CEREAL_REGISTER_TYPE(MetaFile);

//CEREAL_REGISTER_TYPE(SubAssetMetaFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, SubAssetMetaFile);

CEREAL_REGISTER_TYPE(ModelMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(SubAssetMetaFile, ModelMeta);

CEREAL_REGISTER_TYPE(TextureMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, TextureMeta);

CEREAL_REGISTER_TYPE(FolderMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, FolderMeta);

CEREAL_REGISTER_TYPE(NotSupportMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, NotSupportMeta);

CEREAL_REGISTER_TYPE(AnimationMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, AnimationMeta);

CEREAL_REGISTER_TYPE(MaterialMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, MaterialMeta);

CEREAL_REGISTER_TYPE(MeshMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, MeshMeta);