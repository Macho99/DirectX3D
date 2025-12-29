#pragma once
#include "Renderer.h"
class Material;
class Texture;

class TessTerrain : public Renderer
{
    using Super = Renderer;
public:
	struct InitInfo
	{
		std::wstring heightMapFilename;
		std::wstring layerMapFilename0;
		std::wstring layerMapFilename1;
		std::wstring layerMapFilename2;
		std::wstring layerMapFilename3;
		std::wstring layerMapFilename4;
		std::wstring blendMapFilename;
		float heightScale;
		uint32 heightmapWidth;
		uint32 heightmapHeight;
		float cellSpacing;
	};

	TessTerrain();
	~TessTerrain();

	float GetWidth() const;
	float GetDepth() const;
	float GetHeight(float x, float z) const;

	void Init(const InitInfo& initInfo);
	void InnerRender(RenderTech renderTech) override;
    ID3D11ShaderResourceView* GetLayerMapArraySRV() { return _layerMapArraySRV.Get(); }
    ID3D11ShaderResourceView* GetBlendMapSRV() { return _blendMapTexture->GetComPtr().Get(); }

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

private:

	// Divide heightmap into patches such that each patch has CellsPerPatch cells
	// and CellsPerPatch+1 vertices.  Use 64 so that if we tessellate all the way 
	// to 64, we use all the data from the heightmap.  
	static const int CellsPerPatch = 64;

	ComPtr<ID3D11Buffer> _quadPatchVB;
	ComPtr<ID3D11Buffer> _quadPatchIB;

	ComPtr<ID3D11ShaderResourceView> _layerMapArraySRV;
	shared_ptr<Texture> _blendMapTexture;
    shared_ptr<Texture> _heightMapTexture;
	ComPtr<ID3D11ShaderResourceView> _heightMapSRV;

    TerrainDesc _terrainDesc;

	InitInfo _info;

	uint32 _numPatchVertices = 0;
	uint32 _numPatchQuadFaces = 0;
	uint32 _numPatchVertRows = 0;
	uint32 _numPatchVertCols = 0;

	shared_ptr<Material> _mat;

	std::vector<XMFLOAT2> _patchBoundsY;
	std::vector<float> _heightmap;
};