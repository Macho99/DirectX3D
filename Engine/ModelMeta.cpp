#include "pch.h"
#include "ModelMeta.h"
#include "Converter.h"
#include <regex>

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
    converter.TryExportAll(absPath, GetArtifactPath(), _subAssets, OUT exported);

    _subAssets = exported;
}