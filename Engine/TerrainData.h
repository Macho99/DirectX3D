#pragma once
#include "ResourceBase.h"
class TerrainData : public ResourceBase
{
    using Super = ResourceBase;
public:
    TerrainData();
    ~TerrainData();

	virtual bool OnGUI(bool isReadOnly) override;

    ID3D11ShaderResourceView* GetLayerMapArraySRV();

    fs::path GetHeightMapPath() const;
    ResourceRef<Texture> GetBlendMap() const { return blendMap; }
    float GetHeightScale() const { return heightScale; }
    float GetHeightmapWidth() const { return heightmapWidth; }
    float GetHeightmapHeight() const { return heightmapHeight; }
    float GetCellSpacing() const { return cellSpacing; }

    static string GetExtension() { return ".terrain"; }

private:
    void AddPath(AssetId assetId, vector<fs::path>& paths);

public:
	template<class Archive>
	void serialize(Archive& ar)
    {
        ar( CEREAL_NVP(heightMap),
            CEREAL_NVP(layerMap0),
            CEREAL_NVP(layerMap1),
            CEREAL_NVP(layerMap2),
            CEREAL_NVP(layerMap3),
            CEREAL_NVP(layerMap4),
            CEREAL_NVP(blendMap),
            CEREAL_NVP(heightScale),
            CEREAL_NVP(heightmapWidth),
            CEREAL_NVP(heightmapHeight),
            CEREAL_NVP(cellSpacing)
        );
	}

private:
	AssetRef heightMap;
	ResourceRef<Texture> layerMap0;
	ResourceRef<Texture> layerMap1;
	ResourceRef<Texture> layerMap2;
	ResourceRef<Texture> layerMap3;
	ResourceRef<Texture> layerMap4;
	ResourceRef<Texture> blendMap;
	float heightScale = 50.0f;
	uint32 heightmapWidth = 2049;
	uint32 heightmapHeight = 2049;
    float cellSpacing = 0.5f;

private:
    //Á÷·ÄÈ­ X
    ComPtr<ID3D11ShaderResourceView> _layerMapArraySRV = nullptr;
};