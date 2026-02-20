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
    Super::Import();

    wstring artifactFoloder = GetArtifactPath();
    Converter converter;
    wstring absPath = GetAbsPath();
    converter.ReadAssetFile(absPath);

    fs::create_directories(artifactFoloder);
    vector<SubAssetInfo> exported;
    converter.TryExportAll(GetArtifactPath(), OUT exported);

    for (int newIdx = 0; newIdx < exported.size(); newIdx++)
    {
        for (int prevIdx = 0; prevIdx < _subAssets.size(); prevIdx++)
        {
            if (exported[newIdx].fileName == _subAssets[prevIdx].fileName
                && exported[newIdx].resourceType == _subAssets[prevIdx].resourceType)
            {
                exported[newIdx].assetId = _subAssets[prevIdx].assetId;
                break;
            }
        }

        if (exported[newIdx].assetId.IsValid() == false)
            exported[newIdx].assetId = AssetId::CreateAssetId();
    }

    _subAssets = exported;
}