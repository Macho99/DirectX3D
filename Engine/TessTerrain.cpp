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

bool TessTerrain::Pick(int32 screenX, int32 screenY, Vec3& pickPos, float& distance)
{
	if (TryInitialize() == false)
		return false;

	TerrainData* terrainData = _terrainData.Resolve();
	if (terrainData == nullptr || _heightmap.empty())
		return false;

	Matrix world = GetTransform()->GetWorldMatrix();
	Matrix view = CUR_SCENE->GetMainCamera()->GetCamera()->GetViewMatrix();
	Matrix projection = CUR_SCENE->GetMainCamera()->GetCamera()->GetProjectionMatrix();

	Viewport& vp = GRAPHICS->GetViewport();
	Vec3 nearPos = vp.Unproject(Vec3(screenX, screenY, 0.0f), world, view, projection);
	Vec3 farPos = vp.Unproject(Vec3(screenX, screenY, 1.0f), world, view, projection);

	Vec3 rayDir = farPos - nearPos;
	rayDir.Normalize();
	Ray localRay(nearPos, rayDir);

	const float minHeight = _minHeight;
    const float maxHeight = _maxHeight;
    const float width = GetWidth();
    const float depth = GetDepth();

	BoundingBox localBounds(
		Vec3(0.0f, (minHeight + maxHeight) * 0.5f, 0.0f),
		Vec3(width * 0.5f, (maxHeight - minHeight) * 0.5f, depth * 0.5f));

	float entryDistance = 0.0f;
	if (localRay.Intersects(localBounds, OUT entryDistance) == false)
		return false;

	float marchStart = max(0.0f, entryDistance);
	float marchLength = sqrtf(width * width + depth * depth + (maxHeight - minHeight) * (maxHeight - minHeight));
	float step = max(terrainData->GetCellSpacing() * 0.5f, 0.5f);

	auto SampleDelta = [&](float t)
		{
			Vec3 samplePos = localRay.position + localRay.direction * t;
			const float halfWidth = width * 0.5f;
			const float halfDepth = depth * 0.5f;
			const float epsilon = 0.001f;
			if (samplePos.x < -halfWidth || samplePos.x >= halfWidth - epsilon || samplePos.z <= -halfDepth + epsilon || samplePos.z > halfDepth)
				return FLT_MAX;

			return samplePos.y - GetHeight(samplePos.x, samplePos.z);
		};

	float prevT = marchStart;
	float prevDelta = SampleDelta(prevT);
	if (prevDelta == FLT_MAX)
		return false;

	if (prevDelta <= 0.0f)
	{
		distance = prevT;
		pickPos = localRay.position + localRay.direction * distance;
		return true;
	}

    int count = 0;
	float marchEnd = marchStart + marchLength;
	for (float currentT = marchStart + step; currentT <= marchEnd; currentT += step)
	{
		count++;
		float currentDelta = SampleDelta(currentT);
		if (currentDelta == FLT_MAX)
			break;

		if (currentDelta <= 0.0f)
		{
			float low = prevT;
			float high = currentT;
			for (int32 i = 0; i < 8; ++i)
			{
				float mid = (low + high) * 0.5f;
				float midDelta = SampleDelta(mid);
				if (midDelta <= 0.0f)
					high = mid;
				else
					low = mid;
			}

			distance = high;
			pickPos = localRay.position + localRay.direction * distance;
            //DBG->Log(Utils::Format("March steps: %d, Binary search steps: 8", count));
			return true;
		}

		prevT = currentT;
		prevDelta = currentDelta;
	}

	return false;
}

bool TessTerrain::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
	ImGui::Separator();
    changed |= OnGUIUtils::DrawResourceRef("Terrain Data", _terrainData, false);

	auto DrawModeButton = [this](const char* label, EditMode editMode)
		{
			bool selected = (_editMode == editMode);

			if (!selected)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			}

			if (ImGui::Button(label))
			{
				if (_editMode == editMode)
					_editMode = EditMode::None;
				else
					_editMode = editMode;
			}

			if (!selected)
				ImGui::PopStyleColor(3);
		};

    DrawModeButton("Raise/Lower", EditMode::RaiseLower);
    ImGui::SameLine();
    DrawModeButton("Smooth", EditMode::Smooth);
    ImGui::SameLine();
    DrawModeButton("Texture", EditMode::Texture);
	ImGui::Spacing();

	TerrainData* terrainData = _terrainData.Resolve();
	if (_editMode == EditMode::Texture && terrainData != nullptr)
	{
		ImGui::Text("Selected Blend Layer:");
		auto DrawLayerButton = [this](const char* label, ResourceRef<Texture> textureRef, int num)
			{
				Texture* texture = textureRef.Resolve();
                if (texture == nullptr)
                    return;
				
                bool isSelected = (_selectedBlendLayer == (BlendLayer)num);
				if (isSelected)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
				}

                if (ImGui::ImageButton(label, texture->GetComPtr().Get(), ImVec2(64, 64)))
                {
                    _selectedBlendLayer = (BlendLayer)num;
                }

				if (isSelected)
					ImGui::PopStyleColor(3);

				ImGui::SameLine();
			};
#define X(name, color, num) DrawLayerButton(#name, terrainData->Get##name##(), num);
		BLEND_LAYER_LIST
#undef X
		ImGui::NewLine();
	}

	changed |= OnGUIUtils::DrawResourceRef("Brush", _brushTexture, false);
	changed |= OnGUIUtils::DrawFloat("Brush Radius", &_brushRadius, 1.0f, false);
	changed |= OnGUIUtils::DrawFloat("Brush Strength", &_brushStrength, 0.2f, false);
	//changed |= OnGUIUtils::DrawEnumCombo("Selected Blend Layer", _selectedBlendLayer, BlendLayerNames, (int) BlendLayer::Max);

    short wheelDelta = INPUT->GetMouseWheelDelta();
	if (wheelDelta != 0)
	{
        if (INPUT->GetButton(KEY_TYPE::LSHIFT))
        {
            _brushStrength -= wheelDelta * 0.02f;
            _brushStrength = max(0.0f, _brushStrength);
        }
        else
        {
            _brushRadius -= wheelDelta * 0.02f;
            _brushRadius = max(0.01f, _brushRadius);
        }
        changed = true;
	}

	bool curHeightmapEditing = false;
	if (_terrainDesc.brushRadius > 0.01f)
	{
		int directionY = 0;
		if (INPUT->GetButton(KEY_TYPE::LBUTTON))
		{
			if (INPUT->GetButton(KEY_TYPE::LSHIFT))
			{
                directionY = -1;
			}
			else
			{
				directionY = 1;
			}
		}
		
		if (directionY != 0)
		{
			TerrainData* terrainData = _terrainData.Resolve();
            Texture* brushTexture = _brushTexture.Resolve();
			const DirectX::Image* brushImage = brushTexture->GetInfo().GetImage(0, 0, 0);
            Vec3 pickPos = _terrainDesc.brushPos;
			if (terrainData != nullptr && brushImage != nullptr)
			{
				const float cellSpacing = terrainData->GetCellSpacing();
				const uint32 heightmapWidth = terrainData->GetHeightmapWidth();
				const uint32 heightmapHeight = terrainData->GetHeightmapHeight();
				const float terrainWidth = GetWidth();
				const float terrainDepth = GetDepth();

				const float centerU = (terrainWidth * 0.5f + pickPos.x) / terrainWidth;
				const float centerV = (terrainDepth * 0.5f - pickPos.z) / terrainDepth;
				const int32 centerX = static_cast<int32>(centerU * (heightmapWidth - 1));
				const int32 centerY = static_cast<int32>(centerV * (heightmapHeight - 1));
				const int32 radiusCells = static_cast<int32>(ceilf(_brushRadius / max(cellSpacing, 0.001f)));

				const int32 startY = max<int32>(0, centerY - radiusCells);
				const int32 endY = min<int32>(static_cast<int32>(heightmapHeight) - 1, centerY + radiusCells);
				const int32 startX = max<int32>(0, centerX - radiusCells);
				const int32 endX = min<int32>(static_cast<int32>(heightmapWidth) - 1, centerX + radiusCells);
                if (_editMode == EditMode::Texture)
                {
					Texture* blendMap = terrainData->GetBlendMap().Resolve();
					Vec2 blendMapSize = blendMap->GetSize();
					blendMap->SetDynamic();

                    const int32 startPixelX = (float)startX / heightmapWidth * blendMapSize.x;
                    const int32 endPixelX = (float)endX / heightmapWidth * blendMapSize.x;
                    const int32 startPixelY = (float)startY / heightmapHeight * blendMapSize.y;
                    const int32 endPixelY = (float)endY / heightmapHeight * blendMapSize.y;

					for (int32 y = startPixelY; y <= endPixelY; ++y)
					{
                        const int32 curY = (float)y / blendMapSize.y * heightmapHeight;
                        const float vertexZ = terrainDepth * 0.5f - curY * cellSpacing;
                        for (int32 x = startPixelX; x <= endPixelX; ++x)
                        {
                            const int32 curX = (float)x / blendMapSize.x * heightmapWidth;
                            const float vertexX = -terrainWidth * 0.5f + curX * cellSpacing;
                            const float brushU = (vertexX - pickPos.x) / _brushRadius + 0.5f;
                            const float brushV = (vertexZ - pickPos.z) / _brushRadius + 0.5f;
                            const float brushWeight = SampleBrush(brushImage, brushU, brushV);
                            if (brushWeight <= 0.0f)
                                continue;

                            Color color;
                            bool getResult = blendMap->TryGetPixel(x, y, OUT color);
                            ASSERT(getResult);
							//DBG->Log(Utils::Format("SetPixel x: %d, y: %d, color: (%.2f, %.2f, %.2f, %.2f)", x, y, color.x, color.y, color.z, color.w));

							float power = DT * brushWeight * _brushStrength;
                            Color targetColor = BlendLayerColors[(int)_selectedBlendLayer];
                            color = MathUtils::Lerp(color, targetColor, power);
							
                            //DBG->Log(Utils::Format("SetPixel x: %d, y: %d, color: (%.2f, %.2f, %.2f, %.2f)", x, y, color.x, color.y, color.z, color.w));
							
                            bool setResult = blendMap->TrySetDynamicPixel(x, y, color);
                            ASSERT(setResult);
                        }
					}

					blendMap->ApplyDynamicImageToGPU();
                }
				else
				{
					for (int32 y = startY; y <= endY; ++y)
					{
						const float vertexZ = terrainDepth * 0.5f - y * cellSpacing;
						for (int32 x = startX; x <= endX; ++x)
						{
							const float vertexX = -terrainWidth * 0.5f + x * cellSpacing;
							const float brushU = (vertexX - pickPos.x) / _brushRadius + 0.5f;
							const float brushV = (vertexZ - pickPos.z) / _brushRadius + 0.5f;
							const float brushWeight = SampleBrush(brushImage, brushU, brushV);
							if (brushWeight <= 0.0f)
								continue;

							float power = DT * brushWeight * _brushStrength;
							int index = y * heightmapWidth + x;
							if (_editMode == EditMode::RaiseLower)
							{
								_heightmap[index] += directionY * power;
							}
							else if (_editMode == EditMode::Smooth)
							{
								_heightmap[index] = MathUtils::Lerp(_heightmap[index], Average(y, x), power);
							}
							_halfHeightmap[index] = MathUtils::ConvertFloatToHalf(_heightmap[index]);
						}
					}

					curHeightmapEditing = true;
					UpdateHeightmapTexture();
				}
			}
		}
	}

    if (curHeightmapEditing == false && _prevHeightmapEditing == true)
    {
		CalcAllPatchBoundsY();
		UpdateQuadPatchVB();
        OnHeightmapChanged.Invoke();
    }

	_prevHeightmapEditing = curHeightmapEditing;

    return changed;
}

bool TessTerrain::TryInitialize()
{
    if (_initialized)
        return true;

    TerrainData* terrainData = _terrainData.Resolve();
    if (terrainData == nullptr)
        return false;

	// Divide heightmap into patches such that each patch has CellsPerPatch.
	_numPatchVertRows = ((terrainData->GetHeightmapHeight() - 1) / CellsPerPatch) + 1;
	_numPatchVertCols = ((terrainData->GetHeightmapWidth() - 1) / CellsPerPatch) + 1;

	_numPatchVertices = _numPatchVertRows * _numPatchVertCols;
	_numPatchQuadFaces = (_numPatchVertRows - 1) * (_numPatchVertCols - 1);

	LoadHeightmap();
	//Smooth();
	CalcAllPatchBoundsY();
	BuildQuadPatchVB();
	BuildQuadPatchIB();
	BuildHeightmapSRV();

	_layerMapArraySRV = terrainData->GetLayerMapArraySRV();
	_blendMapTexture = terrainData->GetBlendMap();

	_initialized = true;
    return true;
}

void TessTerrain::OnInspectorFocus()
{
	Super::OnInspectorFocus();
}

void TessTerrain::OnInspectorFocusLost()
{
	Super::OnInspectorFocusLost();

    _editMode = EditMode::None;
    ASSERT(_prevHeightmapEditing == false);
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

	bool useBrush = false;
	if (_editMode != EditMode::None && renderTech == RenderTech::Draw && INPUT->IsMouseInScene())
	{
		auto brushTex = _brushTexture.Resolve();
		if (brushTex != nullptr)
		{
			POINT mousePos = INPUT->GetMousePos();
			float distance;
			if (Pick(mousePos.x, mousePos.y, OUT _terrainDesc.brushPos, OUT distance))
			{
				useBrush = true;
			}
		}
	}
	
    if (useBrush == false)
	{
		_terrainDesc.brushRadius = 0.f;
		//material->SetNormalMap(RESOURCES->GetDummyTexture());
    }
    else
    {
        _terrainDesc.brushRadius = _brushRadius;
        material->SetNormalMap(_brushTexture);
    }

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

	// Store axis-aligned bounding box y-bounds in upper-left patch corner.
	for (uint32 i = 0; i < _numPatchVertRows - 1; ++i)
	{
		for (uint32 j = 0; j < _numPatchVertCols - 1; ++j)
		{
			uint32 patchID = i * (_numPatchVertCols - 1) + j;
			_patchVertices[i * _numPatchVertCols + j].BoundsY = _patchBoundsY[patchID];
		}
	}

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
	_halfHeightmap.resize(_heightmap.size());
	std::transform(_heightmap.begin(), _heightmap.end(), _halfHeightmap.begin(), MathUtils::ConvertFloatToHalf);
	auto [minIt, maxIt] = std::minmax_element(_heightmap.begin(), _heightmap.end());
    _minHeight = *minIt;
    _maxHeight = *maxIt;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &_halfHeightmap[0];
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

float TessTerrain::SampleBrush(const DirectX::Image* brushImage, float u, float v)
{
	if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
		return 0.0f;

	const uint32 brushWidth = static_cast<uint32>(brushImage->width);
	const uint32 brushHeight = static_cast<uint32>(brushImage->height);
	if (brushWidth == 0 || brushHeight == 0)
		return 0.0f;

	const uint32 x = min<uint32>(static_cast<uint32>(u * (brushWidth - 1)), brushWidth - 1);
	const uint32 y = min<uint32>(static_cast<uint32>(v * (brushHeight - 1)), brushHeight - 1);
	const size_t pixelSize = max<size_t>(1, BitsPerPixel(brushImage->format) / 8);
	const uint8* pixel = brushImage->pixels + y * brushImage->rowPitch + x * pixelSize;

	switch (brushImage->format)
	{
	case DXGI_FORMAT_R8_UNORM:
		return pixel[0] / 255.0f;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return pixel[0] / 255.0f;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return pixel[2] / 255.0f;
	case DXGI_FORMAT_R16_UNORM:
		return *reinterpret_cast<const uint16*>(pixel) / 65535.0f;
	case DXGI_FORMAT_R32_FLOAT:
		return *reinterpret_cast<const float*>(pixel);
	default:
		return 0.0f;
	}
}

bool TessTerrain::UpdateHeightmapTexture()
{
    TerrainData* terrainData = _terrainData.Resolve();
    if (terrainData == nullptr || _heightMapTexture2D == nullptr || _heightmap.empty())
        return false;

	if (_isHeightmapDirty)
	{
		_halfHeightmap.resize(_heightmap.size());
		std::transform(_heightmap.begin(), _heightmap.end(), _halfHeightmap.begin(), MathUtils::ConvertFloatToHalf);
		auto [minIt, maxIt] = std::minmax_element(_heightmap.begin(), _heightmap.end());
		_minHeight = *minIt;
		_maxHeight = *maxIt;
        _isHeightmapDirty = false;
	}

    D3D11_MAPPED_SUBRESOURCE subResource = {};
    HRESULT hr = DC->Map(_heightMapTexture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
    if (FAILED(hr))
        return false;

    const uint32 rowPitch = terrainData->GetHeightmapWidth() * sizeof(uint16);
    const uint32 rowCount = terrainData->GetHeightmapHeight();
    for (uint32 row = 0; row < rowCount; ++row)
    {
        unsigned char* dstRow = static_cast<unsigned char*>(subResource.pData) + subResource.RowPitch * row;
        const unsigned char* srcRow = reinterpret_cast<const unsigned char*>(_halfHeightmap.data()) + rowPitch * row;
        memcpy(dstRow, srcRow, rowPitch);
    }

    DC->Unmap(_heightMapTexture2D.Get(), 0);
    return true;
}
