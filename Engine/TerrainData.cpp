#include "pch.h"
#include "TerrainData.h"
#include "OnGUIUtils.h"
#include "Utils.h"

TerrainData::TerrainData()
    : Super(ResourceType::TerrainData)
{
}

TerrainData::~TerrainData()
{
}

bool TerrainData::OnGUI(bool isReadOnly)
{
    bool changed = false;
    changed |= Super::OnGUI(isReadOnly);

    changed |= OnGUIUtils::DrawAssetRef("Height Map", heightMap, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Layer Map 0", layerMap0, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Layer Map 1", layerMap1, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Layer Map 2", layerMap2, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Layer Map 3", layerMap3, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Layer Map 4", layerMap4, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Blend Map", blendMap, isReadOnly);
    changed |= OnGUIUtils::DrawFloat("Height Scale", &heightScale, 0.1f, isReadOnly);
    changed |= OnGUIUtils::DrawUInt32("Heightmap Width", &heightmapWidth, 1.f, isReadOnly);
    changed |= OnGUIUtils::DrawUInt32("Heightmap Height", &heightmapHeight, 1.f, isReadOnly);
    changed |= OnGUIUtils::DrawFloat("Cell Spacing", &cellSpacing, 0.1f, isReadOnly);

    return changed;
}

ID3D11ShaderResourceView* TerrainData::GetLayerMapArraySRV()
{
    if (_layerMapArraySRV == nullptr)
    {
        vector<fs::path> filePaths;

        AddPath(layerMap0.GetAssetId(), filePaths);
        AddPath(layerMap1.GetAssetId(), filePaths);
        AddPath(layerMap2.GetAssetId(), filePaths);
        AddPath(layerMap3.GetAssetId(), filePaths);
        AddPath(layerMap4.GetAssetId(), filePaths);
        _layerMapArraySRV = Utils::CreateTexture2DArraySRV(filePaths);
    }
    return _layerMapArraySRV.Get();
}

fs::path TerrainData::GetHeightMapPath() const
{
    MetaFile* meta = nullptr;
    if (RESOURCES->TryGetMetaByAssetId(heightMap.GetAssetId(), OUT meta) == false)
    {
        DBG->LogErrorW(L"TerrainData::GetHeightMapPath: Failed to find meta for assetId: " + heightMap.GetAssetId().ToWString());
        return fs::path();
    }
    return meta->GetImportedAssetPath();
}

void TerrainData::AddPath(AssetId assetId, vector<fs::path>& paths)
{
    MetaFile* meta = nullptr;
    if (RESOURCES->TryGetMetaByAssetId(assetId, OUT meta) == false)
    {
        DBG->LogErrorW(L"TerrainData::AddPath: Failed to find meta for assetId: " + assetId.ToWString());
        return;
    }
    paths.push_back(meta->GetImportedAssetPath());
}
