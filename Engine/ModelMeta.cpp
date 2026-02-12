#include "pch.h"
#include "ModelMeta.h"
#include "Converter.h"

ModelMeta::ModelMeta() : Super(ResourceType::Model)
{
}

ModelMeta::~ModelMeta()
{
}

void ModelMeta::Import()
{
    wstring artifactFoloder = GetArtifactPath();
    Converter converter;
    wstring absPath = GetAbsPath();
    converter.ReadAssetFile(absPath);

    vector<AssetId> assetIds;
    function<wstring()> artifactPathFunc = [artifactFoloder, &assetIds]()
        {
            AssetId newId = AssetId::CreateAssetId();
            assetIds.push_back(newId);
            return artifactFoloder + L"\\" + newId.ToWString();
        };

    fs::create_directories(artifactFoloder);
    vector<ResourceType> exported;
    converter.TryExportAll(artifactPathFunc, OUT exported);

    assert(assetIds.size() == exported.size());
    _subAssets.clear();
    for (size_t i = 0; i < assetIds.size(); i++)
    {
        SubAssetInfo info;
        info.assetId = assetIds[i];
        info.resourceType = exported[i];
        _subAssets.push_back(info);
    }
}
