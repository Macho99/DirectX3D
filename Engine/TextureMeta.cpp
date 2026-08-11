#include "pch.h"
#include "TextureMeta.h"
#include "Texture.h"
#include "OnGUIUtils.h"

namespace
{
    constexpr wchar_t ImportedTextureName[] = L"texture.dds";
    constexpr wchar_t ThumbnailTextureName[] = L"thumbnail.dds";
    constexpr size_t ThumbnailMaxSize = 256;

    HRESULT LoadSourceImage(const fs::path& path, DirectX::TexMetadata& metadata, DirectX::ScratchImage& image)
    {
        const wstring ext = path.extension().wstring();
        if (ext == L".dds" || ext == L".DDS")
            return DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);
        if (ext == L".tga" || ext == L".TGA")
            return DirectX::LoadFromTGAFile(path.c_str(), &metadata, image);
        return DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);
    }

    HRESULT SaveThumbnail(
        const DirectX::ScratchImage& source,
        const DirectX::TexMetadata& metadata,
        const fs::path& outputPath)
    {
        const DirectX::Image* sourceImage = source.GetImage(0, 0, 0);
        if (sourceImage == nullptr)
            return E_FAIL;

        DirectX::ScratchImage decompressed;
        if (DirectX::IsCompressed(sourceImage->format))
        {
            HRESULT hr = DirectX::Decompress(*sourceImage, DXGI_FORMAT_UNKNOWN, decompressed);
            if (FAILED(hr))
                return hr;
            sourceImage = decompressed.GetImage(0, 0, 0);
        }

        const float scale = min(
            1.0f,
            static_cast<float>(ThumbnailMaxSize) /
                static_cast<float>(max(sourceImage->width, sourceImage->height)));
        const size_t scaledWidth = max<size_t>(1, static_cast<size_t>(ceil(sourceImage->width * scale)));
        const size_t scaledHeight = max<size_t>(1, static_cast<size_t>(ceil(sourceImage->height * scale)));
        const size_t thumbnailWidth = max<size_t>(4, (scaledWidth + 3) & ~size_t(3));
        const size_t thumbnailHeight = max<size_t>(4, (scaledHeight + 3) & ~size_t(3));

        DirectX::ScratchImage resized;
        HRESULT hr = DirectX::Resize(
            *sourceImage, thumbnailWidth, thumbnailHeight,
            DirectX::TEX_FILTER_DEFAULT, resized);
        if (FAILED(hr))
            return hr;

        const DXGI_FORMAT thumbnailFormat = DirectX::IsSRGB(metadata.format)
            ? DXGI_FORMAT_BC3_UNORM_SRGB
            : DXGI_FORMAT_BC3_UNORM;
        DirectX::ScratchImage compressed;
        hr = DirectX::Compress(
            *resized.GetImage(0, 0, 0), thumbnailFormat,
            DirectX::TEX_COMPRESS_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, compressed);
        if (FAILED(hr))
            return hr;

        return DirectX::SaveToDDSFile(
            *compressed.GetImage(0, 0, 0),
            DirectX::DDS_FLAGS_NONE, outputPath.c_str());
    }
}

unique_ptr<ResourceBase> TextureMeta::LoadResource(AssetId assetId) const
{
    assert(assetId == GetAssetId() && "TextureMeta::LoadResource: assetId mismatch");

    const fs::path artifactPath = GetImportedAssetPath(assetId);
    unique_ptr<Texture> texture = make_unique<Texture>();
    texture->Load(artifactPath.wstring());
    if (_keepCpuPixels == false)
        texture->ReleaseCpuCopy();
    texture->SetName(GetAssetPath().stem().wstring());
    return texture;
}

fs::path TextureMeta::GetImportedAssetPath(const AssetId& assetId) const
{
    assert(assetId == GetAssetId() && "TextureMeta::GetImportedAssetPath: assetId mismatch");
    return fs::path(GetArtifactPath()) / ImportedTextureName;
}

fs::path TextureMeta::GetThumbnailPath() const
{
    return fs::path(GetArtifactPath()) / ThumbnailTextureName;
}

fs::path TextureMeta::GetThumbnailPathForArtifact(const fs::path& artifactPath)
{
    return artifactPath.parent_path() / (artifactPath.stem().wstring() + L".thumbnail.dds");
}

Texture* TextureMeta::GetIconTexture() const
{
    Texture* thumbnail = Super::GetIconTexture(
        GetResourceType(), GetAssetId(), GetThumbnailPath());
    if (thumbnail != nullptr)
        thumbnail->ReleaseCpuCopy();
    return thumbnail;
}

bool TextureMeta::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= OnGUIUtils::DrawBool("CPU Pixel", &_keepCpuPixels);
    return changed;
}

HRESULT TextureMeta::SaveArtifacts(
    const DirectX::ScratchImage& source,
    const DirectX::TexMetadata& metadata,
    const fs::path& outputPath,
    const fs::path& thumbnailPath,
    bool keepCpuPixels,
    HRESULT* thumbnailResult)
{
    const HRESULT thumbnailHr = SaveThumbnail(source, metadata, thumbnailPath);
    if (thumbnailResult != nullptr)
        *thumbnailResult = thumbnailHr;
    HRESULT hr = S_OK;

    // Terrain editing reads these pixels on the CPU. Keep them in their authored
    // format; all ordinary render textures are block-compressed below.
    if (keepCpuPixels)
    {
        const DirectX::Image* topImage = source.GetImage(0, 0, 0);
        if (topImage == nullptr)
        {
            hr = E_FAIL;
        }
        else if (DirectX::IsCompressed(topImage->format))
        {
            DirectX::ScratchImage decompressed;
            hr = DirectX::Decompress(*topImage, DXGI_FORMAT_UNKNOWN, decompressed);
            if (SUCCEEDED(hr))
            {
                hr = DirectX::SaveToDDSFile(
                    *decompressed.GetImage(0, 0, 0),
                    DirectX::DDS_FLAGS_NONE, outputPath.c_str());
            }
        }
        else
        {
            // CPU editing only uses mip 0. Do not retain an unnecessary mip chain.
            hr = DirectX::SaveToDDSFile(
                *topImage, DirectX::DDS_FLAGS_NONE, outputPath.c_str());
        }
    }
    else if (DirectX::IsCompressed(metadata.format)
        && metadata.width % 4 == 0
        && metadata.height % 4 == 0)
    {
        // Avoid quality loss and preserve cube/array metadata in authored DDS files.
        hr = DirectX::SaveToDDSFile(
            source.GetImages(), source.GetImageCount(), metadata,
            DirectX::DDS_FLAGS_NONE, outputPath.c_str());
    }
    else
    {
        DirectX::ScratchImage decompressedSource;
        const DirectX::Image* sourceImages = source.GetImages();
        size_t sourceImageCount = source.GetImageCount();
        DirectX::TexMetadata sourceMetadata = metadata;

        if (DirectX::IsCompressed(metadata.format))
        {
            hr = DirectX::Decompress(
                source.GetImages(), source.GetImageCount(), metadata,
                DXGI_FORMAT_UNKNOWN, decompressedSource);
            if (SUCCEEDED(hr))
            {
                sourceImages = decompressedSource.GetImages();
                sourceImageCount = decompressedSource.GetImageCount();
                sourceMetadata = decompressedSource.GetMetadata();
            }
        }

        DirectX::ScratchImage resized;
        const size_t alignedWidth = (sourceMetadata.width + 3) & ~size_t(3);
        const size_t alignedHeight = (sourceMetadata.height + 3) & ~size_t(3);
        if (SUCCEEDED(hr)
            && (alignedWidth != sourceMetadata.width || alignedHeight != sourceMetadata.height))
        {
            hr = DirectX::Resize(
                sourceImages, sourceImageCount, sourceMetadata,
                alignedWidth, alignedHeight, DirectX::TEX_FILTER_DEFAULT, resized);
            if (SUCCEEDED(hr))
            {
                sourceImages = resized.GetImages();
                sourceImageCount = resized.GetImageCount();
                sourceMetadata = resized.GetMetadata();
            }
        }

        DirectX::ScratchImage mipChain;
        const DirectX::Image* importImages = sourceImages;
        size_t importImageCount = sourceImageCount;
        DirectX::TexMetadata importMetadata = sourceMetadata;

        if (SUCCEEDED(hr) && sourceMetadata.mipLevels <= 1)
        {
            hr = DirectX::GenerateMipMaps(
                sourceImages, sourceImageCount, sourceMetadata,
                DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
            if (SUCCEEDED(hr))
            {
                importImages = mipChain.GetImages();
                importImageCount = mipChain.GetImageCount();
                importMetadata = mipChain.GetMetadata();
            }
        }

        DirectX::ScratchImage compressed;
        if (SUCCEEDED(hr))
        {
            // BC7's CPU encoder is prohibitively slow in editor Debug builds.
            // BC3 has the same 8-bits-per-pixel memory footprint and compresses
            // much faster while retaining an alpha channel.
            const DXGI_FORMAT artifactFormat = DirectX::IsSRGB(importMetadata.format)
                ? DXGI_FORMAT_BC3_UNORM_SRGB
                : DXGI_FORMAT_BC3_UNORM;
            const auto flags = static_cast<DirectX::TEX_COMPRESS_FLAGS>(
                DirectX::TEX_COMPRESS_PARALLEL | DirectX::TEX_COMPRESS_DITHER);
            hr = DirectX::Compress(
                importImages, importImageCount, importMetadata,
                artifactFormat, flags, DirectX::TEX_THRESHOLD_DEFAULT, compressed);
        }

        if (SUCCEEDED(hr))
        {
            hr = DirectX::SaveToDDSFile(
                compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(),
                DirectX::DDS_FLAGS_NONE, outputPath.c_str());
        }
    }

    return hr;
}

void TextureMeta::Import()
{
    Super::Import();

    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage source;
    HRESULT hr = LoadSourceImage(GetAssetPath(), metadata, source);
    if (FAILED(hr))
    {
        DBG->LogErrorW(L"[TextureMeta] Failed to load source texture: " + GetAssetPath().wstring());
        return;
    }

    const fs::path outputPath = GetImportedAssetPath(GetAssetId());
    HRESULT thumbnailHr = E_FAIL;
    hr = SaveArtifacts(source, metadata, outputPath, GetThumbnailPath(), _keepCpuPixels, &thumbnailHr);
    if (FAILED(hr))
        DBG->LogErrorW(L"[TextureMeta] Failed to build compressed artifact: " + outputPath.wstring());
    if (FAILED(thumbnailHr))
        DBG->LogErrorW(L"[TextureMeta] Failed to build thumbnail artifact: " + GetThumbnailPath().wstring());
}
