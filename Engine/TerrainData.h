#pragma once
#include "ResourceBase.h"

#define BLEND_LAYER_LIST \
    X(Layer0, Color(0,0,0,0), 0) \
    X(Layer1, Color(1,0,0,0), 1) \
    X(Layer2, Color(0,1,0,0), 2) \
    X(Layer3, Color(0,0,1,0), 3) \
    X(Layer4, Color(0,0,0,1), 4) \

#define NORMAL_LAYER_LIST \
    X(Layer0Normal, 0) \
    X(Layer1Normal, 1) \
    X(Layer2Normal, 2) \
    X(Layer3Normal, 3) \
    X(Layer4Normal, 4) \

#define HEIGHT_LAYER_LIST \
    X(Layer0Height, 0) \
    X(Layer1Height, 1) \
    X(Layer2Height, 2) \
    X(Layer3Height, 3) \
    X(Layer4Height, 4) \

enum class BlendLayer
{
#define X(name, color, num) name,
    BLEND_LAYER_LIST
#undef X
    Max
};

static const char* BlendLayerNames[] =
{
#define X(name, color, num) #name,
    BLEND_LAYER_LIST
#undef X
};

static const Color BlendLayerColors[] =
{
    #define X(name, color, num) color,
    BLEND_LAYER_LIST
    #undef X
};

class TerrainData : public ResourceBase
{
    using Super = ResourceBase;
public:
    static constexpr ResourceType StaticType = ResourceType::TerrainData;
    virtual int GetVersion() const override { return 2; }
    TerrainData();
    ~TerrainData();

	virtual bool OnGUI(bool isReadOnly) override;

    ID3D11ShaderResourceView* GetLayerMapArraySRV();
    ID3D11ShaderResourceView* GetLayerNormalMapArraySRV();
    ID3D11ShaderResourceView* GetLayerHeightMapArraySRV();

    fs::path GetHeightMapPath() const;
    ResourceRef<Texture> GetBlendMap() const { return blendMap; }
    float GetHeightScale() const { return heightScale; }
    float GetHeightmapWidth() const { return heightmapWidth; }
    float GetHeightmapHeight() const { return heightmapHeight; }
    float GetCellSpacing() const { return cellSpacing; }
#define X(name, color, num) ResourceRef<Texture> Get##name##() const { return name; }
    BLEND_LAYER_LIST
#undef X
#define X(name, num) ResourceRef<Texture> Get##name##() const { return name; }
    NORMAL_LAYER_LIST
#undef X
#define X(name, num) ResourceRef<Texture> Get##name##() const { return name; }
    HEIGHT_LAYER_LIST
#undef X
    float GetLayerHeightStrength() const { return layerHeightStrength; }
    float GetLayerHeightNearDistance() const { return layerHeightNearDistance; }
    float GetLayerHeightFarDistance() const { return layerHeightFarDistance; }
    static string GetExtension() { return ".terrain"; }

private:
    bool AddPath(AssetId assetId, vector<fs::path>& paths);

public:
	template<class Archive>
	void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(heightMap));
#define X(name, color, num) ar(CEREAL_NVP(name));
            BLEND_LAYER_LIST
#undef X
        if (Archive::is_saving::value || _version >= 1)
        {
#define X(name, num) ar(CEREAL_NVP(name));
            NORMAL_LAYER_LIST
#undef X
        }
        if (Archive::is_saving::value || _version >= 2)
        {
#define X(name, num) ar(CEREAL_NVP(name));
            HEIGHT_LAYER_LIST
#undef X
            ar(CEREAL_NVP(layerHeightStrength));
            ar(CEREAL_NVP(layerHeightNearDistance));
            ar(CEREAL_NVP(layerHeightFarDistance));
        }
        ar(CEREAL_NVP(blendMap));
        ar(CEREAL_NVP(heightScale));
        ar(CEREAL_NVP(heightmapWidth));
        ar(CEREAL_NVP(heightmapHeight));
        ar(CEREAL_NVP(cellSpacing));
	}

private:
	AssetRef heightMap;
#define X(name, color, num) ResourceRef<Texture> name;
    BLEND_LAYER_LIST
#undef X
#define X(name, num) ResourceRef<Texture> name;
    NORMAL_LAYER_LIST
#undef X
#define X(name, num) ResourceRef<Texture> name;
    HEIGHT_LAYER_LIST
#undef X
	ResourceRef<Texture> blendMap;
	float layerHeightStrength = 0.25f;
	float layerHeightNearDistance = 20.0f;
	float layerHeightFarDistance = 80.0f;
	float heightScale = 50.0f;
	uint32 heightmapWidth = 2049;
	uint32 heightmapHeight = 2049;
    float cellSpacing = 0.5f;

private:
    // Runtime-only resources.
    ComPtr<ID3D11ShaderResourceView> _layerMapArraySRV = nullptr;
    ComPtr<ID3D11ShaderResourceView> _layerNormalMapArraySRV = nullptr;
    ComPtr<ID3D11ShaderResourceView> _layerHeightMapArraySRV = nullptr;
};
