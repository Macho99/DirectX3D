#include "pch.h"
#include "ModelSourceMeta.h"
#include "Converter.h"
#include <regex>
#include "Model.h"
#include "ContentBrowser.h"
#include "FileUtils.h"
#include "OnGUIUtils.h"

ModelSourceMeta::ModelSourceMeta() : Super(ResourceType::Model)
{
}

ModelSourceMeta::~ModelSourceMeta()
{
}

unique_ptr<ResourceBase> ModelSourceMeta::LoadResource(AssetId assetId) const
{
    if (assetId != _assetId)
    {
        return Super::LoadResource(assetId);
    }

    vector<ResourceRef<Material>> materialRefs;
    ResourceRef<ModelMeshResource> meshRef;
    vector<ResourceRef<ModelAnimation>> animationRefs;

    for (const SubAssetInfo& subAsset : _subAssets)
    {
        if (subAsset.resourceType == ResourceType::ModelMesh)
        {
            meshRef = ResourceRef<ModelMeshResource>(subAsset.assetId);
        }
        else if (subAsset.resourceType == ResourceType::Material)
        {
            materialRefs.push_back(ResourceRef<Material>(subAsset.assetId));
        }
        else if (subAsset.resourceType == ResourceType::Animation)
        {
            animationRefs.push_back(ResourceRef<ModelAnimation>(subAsset.assetId));
        }
    }
    unique_ptr<Model> model = make_unique<Model>(materialRefs, meshRef, animationRefs);
    model->SetName(GetAssetPath().stem());
    return model;
}

bool ModelSourceMeta::OnGUI()
{
    bool changed = Super::OnGUI();
    int presetIndex = 0;
    if (_importSettings.preset == ModelImportPreset::Unreal)
        presetIndex = 1;
    else if (_importSettings.preset == ModelImportPreset::Custom)
        presetIndex = 2;

    const int previousPresetIndex = presetIndex;
    bool importSettingsChanged = OnGUIUtils::DrawEnumCombo(
        "Import Preset",
        presetIndex,
        ModelImportPresetNames,
        _countof(ModelImportPresetNames));
    if (presetIndex != previousPresetIndex)
    {
        if (presetIndex == 1)
            _importSettings.preset = ModelImportPreset::Unreal;
        else if (presetIndex == 2)
            _importSettings.preset = ModelImportPreset::Custom;
        else
            _importSettings.preset = ModelImportPreset::Default;
    }

    if (_importSettings.preset == ModelImportPreset::Custom)
    {
        importSettingsChanged |= OnGUIUtils::DrawFloat("Import Scale", &_importSettings.scale, 0.01f);
        importSettingsChanged |= OnGUIUtils::DrawVec3("Import Rotation", &_importSettings.rotation, 1.0f);
        _importSettings.scale = max(_importSettings.scale, 0.0001f);
    }
    else if (_importSettings.preset == ModelImportPreset::Unreal)
    {
        ImGui::TextDisabled("Scale 0.01, Rotation X 90 deg");
    }

    if (ImGui::Button("Reimport"))
    {
        ForceReimport();
        changed = true;
    }

    return changed;
}

void ModelSourceMeta::OnMenu()
{
    Super::OnMenu();

    if (ImGui::MenuItem("Create Model Asset"))
    {
        unique_ptr<ResourceBase> model = LoadResource(_assetId);
        fs::path newAssetPath;
        if (ContentBrowser::TryGetNewFilePath(GetAssetPath().parent_path(), Utils::ToString(model->GetName()), Model::GetExtension(), OUT newAssetPath))
        {
            FileUtils::SaveResourceToJson(newAssetPath, model);
        }
    }
}

void ModelSourceMeta::Import()
{
    Super::Import();

    wstring artifactFoloder = GetArtifactPath();
    Converter converter;
    wstring absPath = GetAssetPath();
    converter.ReadAssetFile(absPath, _importSettings);

    fs::create_directories(artifactFoloder);
    vector<SubAssetInfo> exported;
    converter.TryExportAll(absPath, GetArtifactPath(), _subAssets, OUT exported);

    _subAssets = exported;
}

int ModelSourceMeta::GetVersion() const
{
    return 15;
}
