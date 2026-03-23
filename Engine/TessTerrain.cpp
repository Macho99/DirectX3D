#include "pch.h"
#include "TessTerrain.h"
#include "Utils.h"
#include "Material.h"
#include "Texture.h"
#include <fstream>
#include "VertexData.h"
#include "MathUtils.h"
#include "Camera.h"
#include "OnGUIUtils.h"

TessTerrain::TessTerrain() : Super(StaticType)
{
    //_mat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    //_mat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    //_mat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 64.0f);
    //_mat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

    _heightMapTexture = RESOURCES->AllocateTempResource(make_unique<Texture>());
}

TessTerrain::~TessTerrain()
{
}

float TessTerrain::GetWidth() const
{
    // Total terrain width.
    TerrainData* terrainData = _terrainData.Resolve();
    if (terrainData == nullptr)
        return 0.0f;

    return (terrainData->GetHeightmapWidth() - 1) * terrainData->GetCellSpacing();
}

float TessTerrain::GetDepth() const
{
    // Total terrain depth.
	TerrainData* terrainData = _terrainData.Resolve();
	if (terrainData == nullptr)
		return 0.0f;
    return (terrainData->GetHeightmapHeight() - 1) * terrainData->GetCellSpacing();
}

float TessTerrain::GetHeight(float x, float z) const
{
	TerrainData* terrainData = _terrainData.Resolve();
	if (terrainData == nullptr)
		return 0.0f;

    float cellSpacing = terrainData->GetCellSpacing();
    float heightmapWidth = terrainData->GetHeightmapWidth();
    float heightmapHeight = terrainData->GetHeightmapHeight();

	// Transform from terrain local space to "cell" space.
	float c = (x + 0.5f * GetWidth()) / cellSpacing;
	float d = (z - 0.5f * GetDepth()) / -cellSpacing;

	// Get the row and column we are in.
	int row = (int)floorf(d);
	int col = (int)floorf(c);

	// Grab the heights of the cell we are in.
	// A*--*B
	//  | /|
	//  |/ |
	// C*--*D
	float A = _heightmap[row * heightmapWidth + col];
	float B = _heightmap[row * heightmapWidth + col + 1];
	float C = _heightmap[(row + 1) * heightmapWidth + col];
	float D = _heightmap[(row + 1) * heightmapWidth + col + 1];

	// Where we are relative to the cell.
	float s = c - (float)col;
	float t = d - (float)row;

	// If upper triangle ABC.
	if (s + t <= 1.0f)
	{
		float uy = B - A;
		float vy = C - A;
		return A + s * uy + t * vy;
	}
	else // lower triangle DCB.
	{
		float uy = C - D;
		float vy = B - D;
		return D + (1.0f - s) * uy + (1.0f - t) * vy;
	}
}

bool TessTerrain::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
	ImGui::Separator();
    changed |= OnGUIUtils::DrawResourceRef("Terrain Data", _terrainData, false);

	if (ImGui::Button("Add"))
	{
        DBG->Log("Add height");
		TerrainData* terrainData = _terrainData.Resolve();
		float heightmapHeight = terrainData->GetHeightmapHeight();
		float heightmapWidth = terrainData->GetHeightmapWidth();

		int radius = 10;
		for (uint32 i = heightmapHeight / 2 - radius; i < heightmapHeight / 2 + radius; ++i)
		{
			for (uint32 j = heightmapWidth / 2 - radius; j < heightmapWidth / 2 + radius; ++j)
			{
                float dist = radius - Vec2::Distance(Vec2((float)j, (float)i), Vec2(heightmapWidth / 2.0f, heightmapHeight / 2.0f));

				_heightmap[i * heightmapWidth + j] += dist;
			}
		}
		
		UpdateHeightmapTexture();
        DBG->Log("Add height UpdateHeightmapTexture");

		UpdateQuadPatchVB();
		DBG->Log("Add height End");
	}

    return changed;
}

bool TessTerrain::TryInitialize()
{
    if (_initialized)
        return true;

	DBG->LogW(L"초기화 시작");

    TerrainData* terrainData = _terrainData.Resolve();
    if (terrainData == nullptr)
        return false;

	// Divide heightmap into patches such that each patch has CellsPerPatch.
	_numPatchVertRows = ((terrainData->GetHeightmapHeight() - 1) / CellsPerPatch) + 1;
	_numPatchVertCols = ((terrainData->GetHeightmapWidth() - 1) / CellsPerPatch) + 1;

	_numPatchVertices = _numPatchVertRows * _numPatchVertCols;
	_numPatchQuadFaces = (_numPatchVertRows - 1) * (_numPatchVertCols - 1);

	LoadHeightmap();
	DBG->LogW(L"LoadHeightmap");
	//Smooth();
	//DBG->LogW(L"Smooth");
	CalcAllPatchBoundsY();
	DBG->LogW(L"CalcAllPatchBoundsY");

	BuildQuadPatchVB();
	DBG->LogW(L"BuildQuadPatchVB");
	BuildQuadPatchIB();
	DBG->LogW(L"BuildQuadPatchIB");
	BuildHeightmapSRV();
	DBG->LogW(L"BuildHeightmapSRV");

	_layerMapArraySRV = terrainData->GetLayerMapArraySRV();
	_blendMapTexture = terrainData->GetBlendMap();

	_initialized = true;
    return true;
}

void TessTerrain::InnerRender(RenderTech renderTech)
{
	Material* material = GetMaterial().Resolve();
    if (material == nullptr)
        return;

	TerrainData* terrainData = _terrainData.Resolve();
    if (terrainData == nullptr)
        return;

	if (TryInitialize() == false)
		return;

    Super::InnerRender(renderTech);

	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	//DC->IASetInputLayout(InputLayouts::Terrain.Get());

	uint32 stride = sizeof(VertexTerrain);
	uint32 offset = 0;
	DC->IASetVertexBuffers(0, 1, _quadPatchVB.GetAddressOf(), &stride, &offset);
	DC->IASetIndexBuffer(_quadPatchIB.Get(), DXGI_FORMAT_R16_UINT, 0);

	Matrix world = GetTransform()->GetWorldMatrix();
	if (renderTech == RenderTech::Shadow)
	{
		Matrix viewProj = Light::S_MatView * Light::S_MatProjection;
		Matrix worldViewProj = world * viewProj;
		MathUtils::ExtractFrustumPlanes(_terrainDesc.gWorldFrustumPlanes, viewProj);
	}
	else
	{
		Matrix viewProj = Camera::S_MatView * Camera::S_MatProjection;
		Matrix worldViewProj = world * viewProj;
		MathUtils::ExtractFrustumPlanes(_terrainDesc.gWorldFrustumPlanes, viewProj);
	}
	
	Shader* shader = material->GetShader();
    _terrainDesc.gTexelCellSpaceU = 1.0f / terrainData->GetHeightmapWidth();
    _terrainDesc.gTexelCellSpaceV = 1.0f / terrainData->GetHeightmapHeight();
    _terrainDesc.gWorldCellSpace = terrainData->GetCellSpacing();
	shader->PushTerrainData(_terrainDesc);

    material->SetLayerMapArraySRV(_layerMapArraySRV);
    material->SetDiffuseMap(_heightMapTexture);
    material->SetSpecularMap(_blendMapTexture);

    shader->DrawIndexed(renderTech, 0, _numPatchQuadFaces * 4);

	//ComPtr<ID3DX11EffectTechnique> tech = Effects::TerrainFX->Light1Tech;
	//D3DX11_TECHNIQUE_DESC techDesc;
	//tech->GetDesc(&techDesc);
	//
	//for (uint32 i = 0; i < techDesc.Passes; ++i)
	//{
	//	ComPtr<ID3DX11EffectPass> pass = tech->GetPassByIndex(i);
	//	pass->Apply(0, DC.Get());
	//
	//	DC->DrawIndexed(_numPatchQuadFaces * 4, 0, 0);
	//}

	// FX sets tessellation stages, but it does not disable them.  So do that here
	// to turn off tessellation.
	DC->HSSetShader(0, 0, 0);
	DC->DSSetShader(0, 0, 0);
}

void TessTerrain::SetTerrainData(const ResourceRef<TerrainData>& terrainData)
{
	_terrainData = terrainData;
    if (_initialized == false)
		TryInitialize();
}

void TessTerrain::LoadHeightmap()
{
    TerrainData* terrainData = _terrainData.Resolve();
	// A height for each vertex
	float heightmapHeight = terrainData->GetHeightmapHeight();
    float heightmapWidth = terrainData->GetHeightmapWidth();
    float heightScale = terrainData->GetHeightScale();
	vector<unsigned char> in(heightmapWidth * heightmapHeight);

	// Open the file.
	ifstream inFile;
	inFile.open(terrainData->GetHeightMapPath().c_str(), std::ios_base::binary);

	if (inFile)
	{
		// Read the RAW bytes.
		inFile.read((char*)&in[0], (std::streamsize)in.size());

		// Done with file.
		inFile.close();
	}

	// Copy the array data into a float array and scale it.
	_heightmap.resize(heightmapHeight * heightmapWidth, 0);

	for (uint32 i = 0; i < heightmapHeight * heightmapWidth; ++i)
	{
		_heightmap[i] = (in[i] / 255.0f) * heightScale;
	}
}

void TessTerrain::Smooth()
{
	TerrainData* terrainData = _terrainData.Resolve();
	float heightmapHeight = terrainData->GetHeightmapHeight();
	float heightmapWidth = terrainData->GetHeightmapWidth();
	std::vector<float> dest(_heightmap.size());

	for (uint32 i = 0; i < heightmapHeight; ++i)
	{
		for (uint32 j = 0; j < heightmapWidth; ++j)
		{
			dest[i * heightmapWidth + j] = Average(i, j);
		}
	}

	// Replace the old heightmap with the filtered one.
	_heightmap = dest;
}

bool TessTerrain::InBounds(int32 i, int32 j)
{
	TerrainData* terrainData = _terrainData.Resolve();
	float heightmapHeight = terrainData->GetHeightmapHeight();
	float heightmapWidth = terrainData->GetHeightmapWidth();

	// True if ij are valid indices; false otherwise.
	return
		i >= 0 && i < (int32)heightmapHeight &&
		j >= 0 && j < (int32)heightmapWidth;
}

float TessTerrain::Average(int32 i, int32 j)
{
	TerrainData* terrainData = _terrainData.Resolve();
	float heightmapWidth = terrainData->GetHeightmapWidth();
	// Function computes the average height of the ij element.
	// It averages itself with its eight neighbor pixels.  Note
	// that if a pixel is missing neighbor, we just don't include it
	// in the average--that is, edge pixels don't have a neighbor pixel.
	//
	// ----------
	// | 1| 2| 3|
	// ----------
	// |4 |ij| 6|
	// ----------
	// | 7| 8| 9|
	// ----------

	float avg = 0.0f;
	float num = 0.0f;

	// Use int to allow negatives.  If we use UINT, @ i=0, m=i-1=UINT_MAX
	// and no iterations of the outer for loop occur.
	for (int32 m = i - 1; m <= i + 1; ++m)
	{
		for (int32 n = j - 1; n <= j + 1; ++n)
		{
			if (InBounds(m, n))
			{
				avg += _heightmap[m * heightmapWidth + n];
				num += 1.0f;
			}
		}
	}

	return avg / num;
}

void TessTerrain::CalcAllPatchBoundsY()
{
	_patchBoundsY.resize(_numPatchQuadFaces);

	// For each patch
	for (uint32 i = 0; i < _numPatchVertRows - 1; ++i)
	{
		for (uint32 j = 0; j < _numPatchVertCols - 1; ++j)
		{
			CalcPatchBoundsY(i, j);
		}
	}
}

void TessTerrain::CalcPatchBoundsY(uint32 i, uint32 j)
{
	TerrainData* terrainData = _terrainData.Resolve();
	float heightmapWidth = terrainData->GetHeightmapWidth();
	// Scan the heightmap values this patch covers and compute the min/max height.

	uint32 x0 = j * CellsPerPatch;
	uint32 x1 = (j + 1) * CellsPerPatch;

	uint32 y0 = i * CellsPerPatch;
	uint32 y1 = (i + 1) * CellsPerPatch;

	float minY = +FLT_MAX;
	float maxY = -FLT_MAX;

	for (uint32 y = y0; y <= y1; ++y)
	{
		for (uint32 x = x0; x <= x1; ++x)
		{
			uint32 k = y * heightmapWidth + x;
			minY = std::min<float>(minY, _heightmap[k]);
			maxY = std::max<float>(maxY, _heightmap[k]);
		}
	}

	uint32 patchID = i * (_numPatchVertCols - 1) + j;
	_patchBoundsY[patchID] = XMFLOAT2(minY, maxY);
}

void TessTerrain::BuildQuadPatchVB()
{
	_patchVertices.clear();
    _patchVertices.resize(_numPatchVertRows * _numPatchVertCols);

	float halfWidth = 0.5f * GetWidth();
	float halfDepth = 0.5f * GetDepth();

	float patchWidth = GetWidth() / (_numPatchVertCols - 1);
	float patchDepth = GetDepth() / (_numPatchVertRows - 1);
	float du = 1.0f / (_numPatchVertCols - 1);
	float dv = 1.0f / (_numPatchVertRows - 1);

	for (uint32 i = 0; i < _numPatchVertRows; ++i)
	{
		float z = halfDepth - i * patchDepth;
		for (uint32 j = 0; j < _numPatchVertCols; ++j)
		{
			float x = -halfWidth + j * patchWidth;

			_patchVertices[i * _numPatchVertCols + j].Pos = XMFLOAT3(x, 0.0f, z);

			// Stretch texture over grid.
			_patchVertices[i * _numPatchVertCols + j].Tex.x = j * du;
			_patchVertices[i * _numPatchVertCols + j].Tex.y = i * dv;
		}
	}

	// Store axis-aligned bounding box y-bounds in upper-left patch corner.
	for (uint32 i = 0; i < _numPatchVertRows - 1; ++i)
	{
		for (uint32 j = 0; j < _numPatchVertCols - 1; ++j)
		{
			uint32 patchID = i * (_numPatchVertCols - 1) + j;
			_patchVertices[i * _numPatchVertCols + j].BoundsY = _patchBoundsY[patchID];
		}
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VertexTerrain) * _patchVertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = _patchVertices.data();
	DX_CREATE_BUFFER(&vbd, &vinitData, _quadPatchVB);
}

bool TessTerrain::UpdateQuadPatchVB()
{
    if (_quadPatchVB == nullptr || _patchVertices.empty())
        return false;

    D3D11_MAPPED_SUBRESOURCE subResource = {};
    HRESULT hr = DC->Map(_quadPatchVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
    if (FAILED(hr))
        return false;

    memcpy(subResource.pData, _patchVertices.data(), sizeof(VertexTerrain) * _patchVertices.size());
    DC->Unmap(_quadPatchVB.Get(), 0);
    return true;
}

void TessTerrain::BuildQuadPatchIB()
{
	std::vector<USHORT> indices(_numPatchQuadFaces * 4); // 4 indices per quad face

	// Iterate over each quad and compute indices.
	int32 k = 0;
	for (uint32 i = 0; i < _numPatchVertRows - 1; ++i)
	{
		for (uint32 j = 0; j < _numPatchVertCols - 1; ++j)
		{
			// Top row of 2x2 quad patch
			indices[k] = i * _numPatchVertCols + j;
			indices[k + 1] = i * _numPatchVertCols + j + 1;

			// Bottom row of 2x2 quad patch
			indices[k + 2] = (i + 1) * _numPatchVertCols + j;
			indices[k + 3] = (i + 1) * _numPatchVertCols + j + 1;

			k += 4; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	DX_CREATE_BUFFER(&ibd, &iinitData, _quadPatchIB);
}

void TessTerrain::BuildHeightmapSRV()
{
	TerrainData* terrainData = _terrainData.Resolve();
	float heightmapWidth = terrainData->GetHeightmapWidth();
    float heightmapHeight = terrainData->GetHeightmapHeight();

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = heightmapWidth;
	texDesc.Height = heightmapHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DYNAMIC;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	texDesc.MiscFlags = 0;

	// HALF is defined in xnamath.h, for storing 16-bit float.
	std::vector<uint16> hmap(_heightmap.size());
	std::transform(_heightmap.begin(), _heightmap.end(), hmap.begin(), MathUtils::ConvertFloatToHalf);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &hmap[0];
	data.SysMemPitch = heightmapWidth * sizeof(uint16);
	data.SysMemSlicePitch = 0;

	DX_CREATE_TEXTURE2D(&texDesc, &data, _heightMapTexture2D);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	DX_CREATE_SRV(_heightMapTexture2D.Get(), &srvDesc, _heightMapSRV);

	_heightMapTexture.Resolve()->SetSRV(_heightMapSRV);
}

bool TessTerrain::UpdateHeightmapTexture()
{
    TerrainData* terrainData = _terrainData.Resolve();
    if (terrainData == nullptr || _heightMapTexture2D == nullptr || _heightmap.empty())
        return false;

    std::vector<uint16> hmap(_heightmap.size());
    std::transform(_heightmap.begin(), _heightmap.end(), hmap.begin(), MathUtils::ConvertFloatToHalf);

    D3D11_MAPPED_SUBRESOURCE subResource = {};
    HRESULT hr = DC->Map(_heightMapTexture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
    if (FAILED(hr))
        return false;

    const uint32 rowPitch = terrainData->GetHeightmapWidth() * sizeof(uint16);
    const uint32 rowCount = terrainData->GetHeightmapHeight();
    for (uint32 row = 0; row < rowCount; ++row)
    {
        unsigned char* dstRow = static_cast<unsigned char*>(subResource.pData) + subResource.RowPitch * row;
        const unsigned char* srcRow = reinterpret_cast<const unsigned char*>(hmap.data()) + rowPitch * row;
        memcpy(dstRow, srcRow, rowPitch);
    }

    DC->Unmap(_heightMapTexture2D.Get(), 0);
    return true;
}
