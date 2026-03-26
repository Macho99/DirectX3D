#include "pch.h"
#include "TerrainData.h"
#include "OnGUIUtils.h"
#include "Utils.h"

TerrainData::TerrainData()
    : Super(StaticType)
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
#define X(name, color, num) changed |= OnGUIUtils::DrawResourceRef(#name, name, isReadOnly);
    BLEND_LAYER_LIST
#undef X
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

#define X(name, color, num) AddPath(name.GetAssetId(), filePaths);
        BLEND_LAYER_LIST
#undef X
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
