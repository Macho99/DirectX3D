#include "pch.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

bool Utils::StartsWith(string str, string comp)
{
	wstring::size_type index = str.find(comp);
	if (index != wstring::npos && index == 0)
		return true;

	return false;
}

bool Utils::StartsWith(wstring str, wstring comp)
{
	wstring::size_type index = str.find(comp);
	if (index != wstring::npos && index == 0)
		return true;

	return false;
}

void Utils::Replace(OUT string& str, string comp, string rep)
{
	string temp = str;

	size_t start_pos = 0;
	while ((start_pos = temp.find(comp, start_pos)) != wstring::npos)
	{
		temp.replace(start_pos, comp.length(), rep);
		start_pos += rep.length();
	}

	str = temp;
}

void Utils::Replace(OUT wstring& str, wstring comp, wstring rep)
{
	wstring temp = str;

	size_t start_pos = 0;
	while ((start_pos = temp.find(comp, start_pos)) != wstring::npos)
	{
		temp.replace(start_pos, comp.length(), rep);
		start_pos += rep.length();
	}

	str = temp;
}

std::wstring Utils::ToWString(string value)
{
	return wstring(value.begin(), value.end());
}

std::string Utils::ToString(wstring value)
{
	return string(value.begin(), value.end());
}

string Utils::ToString(Vec3& value)
{
	return std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z);
}

ComPtr<ID3D11ShaderResourceView> Utils::CreateTexture2DArraySRV(vector<wstring>& filenames)
{	//
	// Load the texture elements individually from file.  These textures
	// won't be used by the GPU (0 bind flags), they are just used to 
	// load the image data from file.  We use the STAGING usage so the
	// CPU can read the resource.
	//
	ComPtr<ID3D11Device> device = DEVICE;
	ComPtr<ID3D11DeviceContext> context = DC;

	uint32 size = filenames.size();

	std::vector<ComPtr<ID3D11Texture2D>> srcTex(size);

	for (uint32 i = 0; i < size; ++i)
	{
		DirectX::TexMetadata md = {};

		DirectX::ScratchImage img;
		HRESULT hr = ::LoadFromDDSFile(filenames[i].c_str(), DDS_FLAGS_NONE, &md, img);
		CHECK(hr);

		hr = ::CreateTextureEx(device.Get(), img.GetImages(), img.GetImageCount(), md,
			D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, 0, false, (ID3D11Resource**)srcTex[i].GetAddressOf());

		CHECK(hr);
	}

	//
	// Create the texture array.  Each element in the texture 
	// array has the same format/dimensions.
	//

	D3D11_TEXTURE2D_DESC texElementDesc;
	srcTex[0]->GetDesc(&texElementDesc);
	//D3D11_TEXTURE2D_DESC texElementDesc1;
	//srcTex[1]->GetDesc(&texElementDesc1);
	//D3D11_TEXTURE2D_DESC texElementDesc2;
	//srcTex[2]->GetDesc(&texElementDesc2);
	//D3D11_TEXTURE2D_DESC texElementDesc3;
	//srcTex[3]->GetDesc(&texElementDesc3);


	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width = texElementDesc.Width;
	texArrayDesc.Height = texElementDesc.Height;
	texArrayDesc.MipLevels = texElementDesc.MipLevels;
	texArrayDesc.ArraySize = size;
	texArrayDesc.Format = texElementDesc.Format;
	texArrayDesc.SampleDesc.Count = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> texArray;
	HR(device->CreateTexture2D(&texArrayDesc, 0, texArray.GetAddressOf()));

	//
	// Copy individual texture elements into texture array.
	//

	// for each texture element...
	for (uint32 texElement = 0; texElement < size; ++texElement)
	{
		// for each mipmap level...
		for (uint32 mipLevel = 0; mipLevel < texElementDesc.MipLevels; ++mipLevel)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			HR(context->Map(srcTex[texElement].Get(), mipLevel, D3D11_MAP_READ, 0, &mappedTex2D));

			context->UpdateSubresource(texArray.Get(),
				::D3D11CalcSubresource(mipLevel, texElement, texElementDesc.MipLevels),
				0, mappedTex2D.pData, mappedTex2D.RowPitch, mappedTex2D.DepthPitch);

			context->Unmap(srcTex[texElement].Get(), mipLevel);
		}
	}

	//
	// Create a resource view to the texture array.
	//

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	ZeroMemory(&viewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = size;

	ComPtr<ID3D11ShaderResourceView> texArraySRV;
	HR(device->CreateShaderResourceView(texArray.Get(), &viewDesc, texArraySRV.GetAddressOf()));

	return texArraySRV;

}

vector<Vec4> Utils::ParseUVText(const wstring& filePath)
{
	std::vector<Vec4> result;

	std::ifstream file(filePath);
	if (!file.is_open())
		return result;

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == ';')
			continue;

		std::stringstream ss(line);

		Vec4 v;
		char comma;

		// float , float , float , float
		if (ss >> v.x >> comma
			>> v.y >> comma
			>> v.z >> comma
			>> v.w)
		{
			result.push_back(v);
		}
	}

	return result;
}
