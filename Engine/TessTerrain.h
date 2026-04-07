#pragma once
#include "Renderer.h"
#include "TerrainData.h"
#include "Event.h"
class Material;
class Texture;

class TessTerrain : public Renderer
{
	enum class EditMode
	{
        None,
        RaiseLower,
        Smooth,
		Texture,
	};

    using Super = Renderer;
    DECLARE_COMPONENT(TessTerrain)
public:
	TessTerrain();
	~TessTerrain();

	float GetWidth() const;
	float GetDepth() const;
	float GetHeight(float x, float z) const;
	bool Pick(int32 screenX, int32 screenY, Vec3& pickPos, float& distance);
    bool UpdateQuadPatchVB();
    bool UpdateHeightmapTexture();

	void InnerRender(RenderTech renderTech) override;
    ID3D11ShaderResourceView* GetLayerMapArraySRV() { return _layerMapArraySRV.Get(); }
    ID3D11ShaderResourceView* GetBlendMapSRV() { return _blendMapTexture.Resolve()->GetComPtr().Get(); }
	void SetTerrainData(const ResourceRef<TerrainData>& terrainData);
	ResourceRef<TerrainData> GetTerrainData() const { return _terrainData; }
    void SetBrushTexture(const ResourceRef<Texture>& brushTexture) { _brushTexture = brushTexture; }

	virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris);
	virtual bool TryInitialize() override;

    virtual void OnInspectorFocus() override;
    virtual void OnInspectorFocusLost() override;

    virtual bool OnGUI() override;
    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_terrainData));
        ar(CEREAL_NVP(_brushTexture));
        ar(CEREAL_NVP(_brushRadius));
        ar(CEREAL_NVP(_brushStrength));
    }

private:
	void LoadHeightmap();
	void Smooth();
	bool InBounds(int32 i, int32 j);
	float Average(int32 i, int32 j);
	void CalcAllPatchBoundsY();
	void CalcPatchBoundsY(uint32 i, uint32 j);
	void BuildQuadPatchVB();
	void BuildQuadPatchIB();
	void BuildHeightmapSRV();

    float SampleBrush(const DirectX::Image* brushImage, float u, float v);

public:
	Event<> OnHeightmapChanged;

private:
	mutable ResourceRef<TerrainData> _terrainData;
    bool _initialized = false;

	// Divide heightmap into patches such that each patch has CellsPerPatch cells
	// and CellsPerPatch+1 vertices.  Use 64 so that if we tessellate all the way
	// to 64, we use all the data from the heightmap.
	static const int CellsPerPatch = 64;

	ComPtr<ID3D11Buffer> _quadPatchVB;
	ComPtr<ID3D11Buffer> _quadPatchIB;
    vector<VertexTerrain> _patchVertices;

	ComPtr<ID3D11ShaderResourceView> _layerMapArraySRV;
	ResourceRef<Texture> _blendMapTexture;
    ResourceRef<Texture> _heightMapTexture;
    ComPtr<ID3D11Texture2D> _heightMapTexture2D;
	ComPtr<ID3D11ShaderResourceView> _heightMapSRV;

    ResourceRef<Texture> _brushTexture;
	EditMode _editMode = EditMode::None;
	float _brushRadius = 20.f;
    float _brushStrength = 5.f;
    BlendLayer _selectedBlendLayer = BlendLayer::Layer0;

    TerrainDesc _terrainDesc;

	uint32 _numPatchVertices = 0;
	uint32 _numPatchQuadFaces = 0;
	uint32 _numPatchVertRows = 0;
	uint32 _numPatchVertCols = 0;

	vector<XMFLOAT2> _patchBoundsY;
	vector<float> _heightmap;
    vector<uint16> _halfHeightmap; // 16-bit heightmap for GPU
    bool _isHeightmapDirty = false;
    bool _prevHeightmapEditing = false;

    float _minHeight = FLT_MAX;
    float _maxHeight = -FLT_MAX;
	uint32 _triCellSize = 3;
};
