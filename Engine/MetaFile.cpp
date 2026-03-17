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
#include "ShaderMeta.h"
#include "TerrainDataMeta.h"
#include "TerrainData.h"

#include "Material.h"
#include "ModelMeshResource.h"
#include "ModelAnimation.h"
#include "EditorManager.h"

#include "fstream"
#include "MetaStore.h"
#include "DndPayload.h"
#include "FileUtils.h"

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

fs::path MetaFile::GetImportedAssetPath() const
{
    // TODO: 빌드 환경 대응
    return GetAssetPath();
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
    bool isManifestRefreshed = _importManifest.Refresh(_assetPath, GetVersion(), isDirty);

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

void MetaFile::ForceReimport()
{
    _importManifest = ImportManifest();
    wstring manifestPath = GetManifestPath();
    if (fs::exists(manifestPath))
        fs::remove(manifestPath);

    ImportIfDirty();
}

bool MetaFile::OnGUI()
{
    if (ImGui::Button("Reimport"))
    {
        ForceReimport();
    }
    return false;
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
    string key;
    fs::path loadPath;

    if (resourceType == ResourceType::Texture)
    {
        key = assetId.ToString();
        loadPath = absPath;
    }
    else
    {
        switch (resourceType)
        {
        case ResourceType::Animation:
            key = "AnimationIcon";
            break;
        case ResourceType::Model:
            key = "ModelIcon";
            break;
        case ResourceType::Folder:
            key = "FolderIcon";
            break;
        case ResourceType::Material:
            key = "MaterialIcon";
            break;
        case ResourceType::Mesh:
            key = "MeshIcon";
            break;
        case ResourceType::TerrainData:
            key = "TerrainIcon";
            break;
        default:
            key = "DefaultFileIcon";
            break;
        }
        loadPath = L"..\\EditorResource\\" + Utils::ToWString(key) + L".png";
    }
    return RESOURCES->GetEditorTexture(key, loadPath);
}

unique_ptr<ResourceBase> MetaFile::LoadResource(ResourceType resourceType, const fs::path& filePath) const
{
    unique_ptr<ResourceBase> resource = nullptr;

    switch (resourceType)
    {
    case ResourceType::Texture:
    {
        unique_ptr<Texture> texture = make_unique<Texture>();
        texture->Load(filePath);
        resource = std::move(texture);
        break;
    }
    case ResourceType::Shader:
        resource = make_unique<Shader>(filePath);
        break;
    case ResourceType::ModelMesh:
    {
        unique_ptr<ModelMeshResource> modelMeshResource = make_unique<ModelMeshResource>();
        modelMeshResource->ReadModel(filePath);
        resource = std::move(modelMeshResource);
        break;
    }
    case ResourceType::Animation:
    {
        unique_ptr<ModelAnimation> animation = make_unique<ModelAnimation>();
        animation->ReadAnimation(filePath);
        resource = std::move(animation);
        break;
    }
    case ResourceType::Material:
    case ResourceType::TerrainData:
    case ResourceType::Mesh:
        resource = FileUtils::LoadResourceFromJson(filePath);
        break;
    }



    if(resource == nullptr)
        ASSERT(false, "MetaFile::LoadResource: Unsupported resource type: " + to_string((int)resourceType));

    resource->SetName(filePath.stem().wstring());
    return resource;
}

unique_ptr<ResourceBase> MetaFile::LoadResource(AssetId assetId) const
{
    return LoadResource(_resourceType, _assetPath);
}

string MetaFile::GetName(const AssetId& assetId)
{
    return GetAssetPath().filename().string();
}

Texture* MetaFile::GetIconTexture() const
{
    return GetIconTexture(GetResourceType(), GetAssetId(), GetAssetPath());
}

void MetaFile::DrawContentBrowserItem(fs::path& currentFolder, float thumbSize, int& curCol, int columns) const
{
    static AssetId renamingAssetId;
    static char renameBuffer[260] = {};
    static bool requestRenameFocus = false;
    const bool isRenaming = (renamingAssetId == _assetId);

    fs::path assetPath = GetAssetPath();
    bool isFolder = (GetResourceType() == ResourceType::Folder);
    std::string name = Utils::ToUtf8(assetPath.filename());

    ImGui::PushID(assetPath.string().c_str());
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

    if (ImGui::BeginPopupContextItem("Context"))
    {
        if (ImGui::MenuItem("Delete"))
        {
            fs::remove(assetPath);
        }
        ImGui::EndPopup();
    }

    AssetRef selectedAsset; 
    int selectedSubAssetIndex;
    EDITOR->TryGetContentBrowserAsset(OUT selectedAsset, OUT selectedSubAssetIndex);
    bool selected = (selectedAsset.GetAssetId() == _assetId);

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
        EDITOR->ClickAsset(_assetId);
    }
    if (isFolder && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        EDITOR->UnselectAsset();
        currentFolder = assetPath;
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

    if (isRenaming)
    {
        ImGui::SetNextItemWidth(tileW);
        if (requestRenameFocus)
        {
            ImGui::SetKeyboardFocusHere();
            requestRenameFocus = false;
        }

        const ImGuiInputTextFlags renameFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
        const bool enterPressed = ImGui::InputText("##rename", renameBuffer, IM_ARRAYSIZE(renameBuffer), renameFlags);

        bool finishRename = enterPressed;
        bool cancelRename = false;

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            cancelRename = true;
            finishRename = false;
        }
        else if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            finishRename = true;
        }

        if (finishRename || cancelRename)
        {
            if (!cancelRename)
            {
                string newName(renameBuffer);
                const auto begin = newName.find_first_not_of(" \t\n\r");
                const auto end = newName.find_last_not_of(" \t\n\r");
                if (begin == string::npos)
                    newName.clear();
                else
                    newName = newName.substr(begin, end - begin + 1);

                newName += assetPath.extension().string(); // 확장자 유지

                if (!newName.empty())
                {
                    fs::path newAbsPath = assetPath;
                    newAbsPath.replace_filename(newName);

                    if (newAbsPath != assetPath)
                    {
                        std::error_code ec;
                        fs::rename(assetPath, newAbsPath, ec);
                        if (ec)
                        {
                            DBG->LogErrorW(L"[MetaFile] Rename failed: " + assetPath.wstring() + L" -> " + newAbsPath.wstring() + L", error=" + Utils::ToWString(ec.message()));
                        }
                    }
                }
            }

            renamingAssetId = AssetId();
        }
    }
    else
    {
        ImGui::TextWrapped("%s", name.c_str());
    }


    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::PopTextWrapPos();

    ImGui::EndGroup();
    ImGui::PopID();

    if (++curCol >= columns)
        curCol = 0;

    if (selected &&
        !isRenaming &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
        ImGui::IsMouseHoveringRect(textPos, textMax))
    {
        renamingAssetId = _assetId;
        const string baseName = assetPath.stem().string();
        strncpy_s(renameBuffer, baseName.c_str(), _TRUNCATE);
        requestRenameFocus = true;
    }

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

CEREAL_REGISTER_TYPE(ShaderMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, ShaderMeta);

CEREAL_REGISTER_TYPE(TerrainDataMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, TerrainDataMeta);