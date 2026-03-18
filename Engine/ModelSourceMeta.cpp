#include "pch.h"
#include "ModelSourceMeta.h"
#include "Converter.h"
#include <regex>
#include "Model.h"

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
    return model;
}

void ModelSourceMeta::Import()
{
    Super::Import();

    wstring artifactFoloder = GetArtifactPath();
    Converter converter;
    wstring absPath = GetAssetPath();
    converter.ReadAssetFile(absPath);

    fs::create_directories(artifactFoloder);
    vector<SubAssetInfo> exported;
    converter.TryExportAll(absPath, GetArtifactPath(), _subAssets, OUT exported);

    _subAssets = exported;
}