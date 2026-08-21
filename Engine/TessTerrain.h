#pragma once
#include "Renderer.h"
#include "TerrainData.h"
#include "Event.h"
#include "cereal/types/utility.hpp"
#include "cereal/types/vector.hpp"
class Material;
class Texture;
class Model;

class TessTerrain : public Renderer
{
	using Super = Renderer;
	DECLARE_COMPONENT(TessTerrain)
public:
	enum class EditMode
	{
		None,
		RaiseLower,
		Smooth,
		Texture,
		Model,
	};

	TessTerrain();
	~TessTerrain();
	virtual int GetVersion() const override { return 2; }

	float GetWidth() const;
	float GetDepth() const;
	float GetHeight(float x, float z) const;
	void SetPositionOffset(const Vec3& positionOffset) { _positionOffset = positionOffset; }
	const Vec3& GetPositionOffset() const { return _positionOffset; }
	bool Pick(int32 screenX, int32 screenY, Vec3& pickPos, float& distance);
	bool UpdateQuadPatchVB();
	bool UpdateHeightmapTexture();
	EditMode GetEditMode() const { return _editMode; }

	void InnerRender(RenderTech renderTech) override;
	ID3D11ShaderResourceView* GetLayerMapArraySRV() { return _layerMapArraySRV.Get(); }
	ID3D11ShaderResourceView* GetLayerNormalMapArraySRV() { return _layerNormalMapArraySRV.Get(); }
	ID3D11ShaderResourceView* GetLayerHeightMapArraySRV() { return _layerHeightMapArraySRV.Get(); }
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
		if (Archive::is_saving::value || _version >= 1)
			ar(CEREAL_NVP(_positionOffset));
		if (Archive::is_saving::value || _version >= 2)
			ar(CEREAL_NVP(_instanceModels));
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

	bool SaveHeightmap();
	bool SaveBlendmap();

	float SampleBrush(const DirectX::Image* brushImage, float u, float v);
	Transform* GetOrCreateModelParent(const ResourceRef<Model>& model);
	bool PlaceModel(const ResourceRef<Model>& model, const Vec3& scale, const Vec3& terrainLocalPosition);
	bool RemoveModels(const Vec3& terrainLocalPosition, float radius);

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
	ComPtr<ID3D11ShaderResourceView> _layerNormalMapArraySRV;
	ComPtr<ID3D11ShaderResourceView> _layerHeightMapArraySRV;
	ResourceRef<Texture> _blendMapTexture;
	ResourceRef<Texture> _heightMapTexture;
	ComPtr<ID3D11Texture2D> _heightMapTexture2D;
	ComPtr<ID3D11ShaderResourceView> _heightMapSRV;

	ResourceRef<Texture> _brushTexture;
	EditMode _editMode = EditMode::None;
	float _brushRadius = 20.f;
	float _brushStrength = 5.f;
	Vec3 _positionOffset = Vec3::Zero;
	BlendLayer _selectedBlendLayer = BlendLayer::Layer0;
	vector<pair<ResourceRef<Model>, Vec3>> _instanceModels;
	int32 _selectedInstanceModel = 0;

	TerrainDesc _terrainDesc;

	uint32 _numPatchVertices = 0;
	uint32 _numPatchQuadFaces = 0;
	uint32 _numPatchVertRows = 0;
	uint32 _numPatchVertCols = 0;

	vector<XMFLOAT2> _patchBoundsY;
	vector<float> _heightmap;
	vector<uint16> _halfHeightmap; // 16-bit heightmap for GPU
	bool _prevHeightmapEditing = false;

	bool _isBlendmapDirty = false;
	bool _isHeightmapDirty = false;
	float _minHeight = FLT_MAX;
	float _maxHeight = -FLT_MAX;
	uint32 _triCellSize = 3;

	bool _submitTrianglesAlways = true;
};
