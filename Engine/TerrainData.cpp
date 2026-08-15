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
    bool layerMapsChanged = false;
    changed |= Super::OnGUI(isReadOnly);

    changed |= OnGUIUtils::DrawAssetRef("Height Map", heightMap, isReadOnly);
    ImGui::Separator();
#define X(name, color, num) layerMapsChanged |= OnGUIUtils::DrawResourceRef(#name, name, isReadOnly);
    BLEND_LAYER_LIST
#undef X
        ImGui::Separator();
#define X(name, num) layerMapsChanged |= OnGUIUtils::DrawResourceRef(#name, name, isReadOnly);
    NORMAL_LAYER_LIST
#undef X
        ImGui::Separator();
#define X(name, num) layerMapsChanged |= OnGUIUtils::DrawResourceRef(#name, name, isReadOnly);
    HEIGHT_LAYER_LIST
#undef X
    changed |= layerMapsChanged;
    ImGui::Separator();
    changed |= OnGUIUtils::DrawFloat("Layer Height Strength", &layerHeightStrength, 0.01f, isReadOnly);
    changed |= OnGUIUtils::DrawFloat("Layer Height Near Distance", &layerHeightNearDistance, 1.0f, isReadOnly);
    changed |= OnGUIUtils::DrawFloat("Layer Height Far Distance", &layerHeightFarDistance, 1.0f, isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Blend Map", blendMap, isReadOnly);
    changed |= OnGUIUtils::DrawFloat("Height Scale", &heightScale, 0.1f, isReadOnly);
    changed |= OnGUIUtils::DrawUInt32("Heightmap Width", &heightmapWidth, 1.f, isReadOnly);
    changed |= OnGUIUtils::DrawUInt32("Heightmap Height", &heightmapHeight, 1.f, isReadOnly);
    changed |= OnGUIUtils::DrawFloat("Cell Spacing", &cellSpacing, 0.1f, isReadOnly);

    layerHeightStrength = max(0.0f, layerHeightStrength);
    layerHeightNearDistance = max(0.0f, layerHeightNearDistance);
    layerHeightFarDistance = max(layerHeightNearDistance + 0.01f, layerHeightFarDistance);

    if (layerMapsChanged)
    {
        _layerMapArraySRV.Reset();
        _layerNormalMapArraySRV.Reset();
        _layerHeightMapArraySRV.Reset();
    }

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

ID3D11ShaderResourceView* TerrainData::GetLayerNormalMapArraySRV()
{
    if (_layerNormalMapArraySRV == nullptr)
    {
        vector<fs::path> filePaths;
        bool hasAllNormalMaps = true;

#define X(name, num) \
        if (name.GetAssetId().IsValid()) \
            hasAllNormalMaps &= AddPath(name.GetAssetId(), filePaths); \
        else \
            hasAllNormalMaps = false;
        NORMAL_LAYER_LIST
#undef X

        if (hasAllNormalMaps)
            _layerNormalMapArraySRV = Utils::CreateTexture2DArraySRV(filePaths);
    }
    return _layerNormalMapArraySRV.Get();
}

ID3D11ShaderResourceView* TerrainData::GetLayerHeightMapArraySRV()
{
    if (_layerHeightMapArraySRV == nullptr)
    {
        vector<fs::path> filePaths;
        bool hasAllHeightMaps = true;

#define X(name, num) \
        if (name.GetAssetId().IsValid()) \
            hasAllHeightMaps &= AddPath(name.GetAssetId(), filePaths); \
        else \
            hasAllHeightMaps = false;
        HEIGHT_LAYER_LIST
#undef X

        if (hasAllHeightMaps)
            _layerHeightMapArraySRV = Utils::CreateTexture2DArraySRV(filePaths);
    }
    return _layerHeightMapArraySRV.Get();
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

bool TerrainData::AddPath(AssetId assetId, vector<fs::path>& paths)
{
    MetaFile* meta = nullptr;
    if (RESOURCES->TryGetMetaByAssetId(assetId, OUT meta) == false)
    {
        DBG->LogErrorW(L"TerrainData::AddPath: Failed to find meta for assetId: " + assetId.ToWString());
        return false;
    }
    paths.push_back(meta->GetImportedAssetPath());
    return true;
}
