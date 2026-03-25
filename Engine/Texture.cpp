#include "pch.h"
#include "Texture.h"

Texture::Texture() : Super(StaticType)
{

}

Texture::~Texture()
{

}

void Texture::Load(const wstring& path)
{
	// 파일 확장자 얻기
	wstring ext = fs::path(path).extension();

	DirectX::TexMetadata md;

	HRESULT hr;
	if (ext == L".dds" || ext == L".DDS")
		hr = ::LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, &md, _img);
	else if (ext == L".tga" || ext == L".TGA")
		hr = ::LoadFromTGAFile(path.c_str(), &md, _img);
	else // png, jpg, jpeg, bmp
		hr = ::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &md, _img);

	CHECK(hr);

	_shaderResourveView.Reset();
	hr = ::CreateShaderResourceView(DEVICE.Get(), _img.GetImages(), _img.GetImageCount(), md, _shaderResourveView.GetAddressOf());
	CHECK(hr);
	
	_size.x = md.width;
	_size.y = md.height;
    _loadedPath = path;
}

ComPtr<ID3D11Texture2D> Texture::GetTexture2D() const
{
	ComPtr<ID3D11Texture2D> texture;
	_shaderResourveView->GetResource((ID3D11Resource**)texture.GetAddressOf());
	return texture;
}

void Texture::SetSRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    _shaderResourveView = srv;
}

bool Texture::SetDynamic()
{
    if (_isDynamic)
        return true;

    if (_img.GetImageCount() == 0)
        return false;

    if (CreateDynamicSRVFromImage() == false)
        return false;

    _isDynamic = true;
    return true;
}

bool Texture::TryGetPixel(uint32 x, uint32 y, Color& outColor) const
{
    const DirectX::Image* image = _img.GetImage(0, 0, 0);
    if (image == nullptr)
        return false;

    if (image == nullptr || x >= image->width || y >= image->height)
        return false;

    const uint8* pixel = image->pixels + y * image->rowPitch;
    switch (image->format)
    {
    case DXGI_FORMAT_R8_UNORM:
    {
        const float value = pixel[x] / 255.0f;
        outColor = Color(value, value, value, 1.0f);
        return true;
    }
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    {
        const uint8* rgba = pixel + (x * 4);
        outColor = Color(rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f);
        return true;
    }    
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    {
        const uint8* bgra = pixel + (x * 4);
        const float alpha = (image->format == DXGI_FORMAT_B8G8R8A8_UNORM) ? (bgra[3] / 255.0f) : 1.0f;
        outColor = Vec4(bgra[2] / 255.0f, bgra[1] / 255.0f, bgra[0] / 255.0f, alpha);
        return true;
    }
    default:
        return false;
    }
}

bool Texture::TrySetDynamicPixel(uint32 x, uint32 y, const Color& color)
{
    if (_isDynamic == false)
        return false;

    const DirectX::Image* image = _img.GetImage(0, 0, 0);
    if (image == nullptr)
        return false;

    if (x >= image->width || y >= image->height)
        return false;

    const auto toByte = [](float value)
        {
            const float clamped = std::clamp(value, 0.0f, 1.0f);
            return static_cast<uint8>(clamped * 255.0f + 0.5f);
        };

    uint8* pixel = image->pixels + y * image->rowPitch;
    switch (image->format)
    {
    case DXGI_FORMAT_R8_UNORM:
    {
        pixel[x] = toByte(color.x);
        return true;
    }
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    {
        uint8* rgba = pixel + (x * 4);
        rgba[0] = toByte(color.x);
        rgba[1] = toByte(color.y);
        rgba[2] = toByte(color.z);
        rgba[3] = toByte(color.w);
        return true;
    }    
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    {
        uint8* bgra = pixel + (x * 4);
        bgra[0] = toByte(color.z);
        bgra[1] = toByte(color.y);
        bgra[2] = toByte(color.x);
        if (image->format == DXGI_FORMAT_B8G8R8A8_UNORM)
            bgra[3] = toByte(color.w);
        return true;
    }
    default:
        return false;
    }
}

bool Texture::SaveAndReload()
{
    if (_loadedPath.empty())
        return false;

    DirectX::ScratchImage snapshot;
    ComPtr<ID3D11Texture2D> texture2D = GetTexture2D();
    if (texture2D == nullptr)
        return false;

    HRESULT hr = DirectX::CaptureTexture(DEVICE.Get(), DC.Get(), texture2D.Get(), snapshot);
    if (FAILED(hr))
        return false;

    if (SaveScratchImageToFile(snapshot) == false)
        return false;

    Load(_loadedPath);
    return true;
}

bool Texture::ApplyDynamicImageToGPU()
{
    if (_isDynamic == false)
        return false;

    const DirectX::Image* image = _img.GetImage(0, 0, 0);
    if (image == nullptr)
        return false;

    ComPtr<ID3D11Texture2D> texture2D = GetTexture2D();
    if (texture2D == nullptr)
        return false;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = DC->Map(texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
        return false;

    const uint32 srcRowPitch = static_cast<uint32>(image->rowPitch);
    const uint32 rowCount = static_cast<uint32>(image->height);
    for (uint32 row = 0; row < rowCount; ++row)
    {
        const uint8* srcRow = image->pixels + srcRowPitch * row;
        uint8* dstRow = static_cast<uint8*>(mapped.pData) + mapped.RowPitch * row;
        memcpy(dstRow, srcRow, srcRowPitch);
    }

    DC->Unmap(texture2D.Get(), 0);
    return true;
}

bool Texture::CreateDynamicSRVFromImage()
{
    const DirectX::TexMetadata md = _img.GetMetadata();
    const DirectX::Image* image = _img.GetImage(0, 0, 0);
    if (image == nullptr)
        return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(md.width);
    desc.Height = static_cast<UINT>(md.height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = md.format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = image->pixels;
    initData.SysMemPitch = static_cast<UINT>(image->rowPitch);
    initData.SysMemSlicePitch = 0;

    ComPtr<ID3D11Texture2D> dynamicTexture;
    DX_CREATE_TEXTURE2D(&desc, &initData, dynamicTexture);

    ComPtr<ID3D11ShaderResourceView> dynamicSRV;
    DX_CREATE_SRV(dynamicTexture.Get(), nullptr, dynamicSRV);

    _shaderResourveView = dynamicSRV;
    return true;
}

bool Texture::SaveScratchImageToFile(const DirectX::ScratchImage& image) const
{
    const wstring ext = fs::path(_loadedPath).extension();
    const DirectX::TexMetadata md = image.GetMetadata();

    HRESULT hr = E_FAIL;
    if (ext == L".dds" || ext == L".DDS")
    {
        hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), md, DDS_FLAGS_NONE, _loadedPath.c_str());
    }
    else if (ext == L".tga" || ext == L".TGA")
    {
        const DirectX::Image* topImage = image.GetImage(0, 0, 0);
        if (topImage == nullptr)
            return false;
        hr = DirectX::SaveToTGAFile(*topImage, _loadedPath.c_str());
    }
    else
    {
        GUID codec = DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG);
        if (ext == L".jpg" || ext == L".JPG" || ext == L".jpeg" || ext == L".JPEG")
            codec = DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG);
        else if (ext == L".bmp" || ext == L".BMP")
            codec = DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP);
        else if (ext == L".tif" || ext == L".TIF" || ext == L".tiff" || ext == L".TIFF")
            codec = DirectX::GetWICCodec(DirectX::WIC_CODEC_TIFF);

        hr = DirectX::SaveToWICFile(
            image.GetImages(),
            image.GetImageCount(),
            WIC_FLAGS_NONE,
            codec,
            _loadedPath.c_str());
    }

    return SUCCEEDED(hr);
}