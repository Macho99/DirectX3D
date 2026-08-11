#include "pch.h"
#include "TextureMeta.h"
#include "Texture.h"
#include "OnGUIUtils.h"

namespace
{
    constexpr wchar_t ImportedTextureName[] = L"texture.dds";
    constexpr wchar_t ThumbnailTextureName[] = L"thumbnail.dds";
    constexpr size_t ThumbnailMaxSize = 256;
    constexpr const char* TextureCompressionFormatNames[] =
    {
        "Uncompressed",
        "BC3",
        "BC7",
    };

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
    bool importSettingsChanged = OnGUIUtils::DrawBool("CPU Pixel", &_keepCpuPixels);
    importSettingsChanged |= OnGUIUtils::DrawEnumCombo(
        "Compression", _compressionFormat,
        TextureCompressionFormatNames, _countof(TextureCompressionFormatNames),
        _keepCpuPixels);

    if (importSettingsChanged)
        ForceReimport();

    changed |= importSettingsChanged;
    return changed;
}

HRESULT TextureMeta::SaveArtifacts(
    const DirectX::ScratchImage& source,
    const DirectX::TexMetadata& metadata,
    const fs::path& outputPath,
    const fs::path& thumbnailPath,
    bool keepCpuPixels,
    HRESULT* thumbnailResult,
    TextureCompressionFormat compressionFormat)
{
    const HRESULT thumbnailHr = SaveThumbnail(source, metadata, thumbnailPath);
    if (thumbnailResult != nullptr)
        *thumbnailResult = thumbnailHr;
    HRESULT hr = S_OK;

    if (compressionFormat >= TextureCompressionFormat::Max)
        compressionFormat = TextureCompressionFormat::BC7;

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
    else
    {
        DXGI_FORMAT artifactFormat = DXGI_FORMAT_UNKNOWN;
        if (compressionFormat == TextureCompressionFormat::BC3)
        {
            artifactFormat = DirectX::IsSRGB(metadata.format)
                ? DXGI_FORMAT_BC3_UNORM_SRGB
                : DXGI_FORMAT_BC3_UNORM;
        }
        else if (compressionFormat == TextureCompressionFormat::BC7)
        {
            artifactFormat = DirectX::IsSRGB(metadata.format)
                ? DXGI_FORMAT_BC7_UNORM_SRGB
                : DXGI_FORMAT_BC7_UNORM;
        }

        // Preserve an authored DDS only when it already matches the selected
        // compression format. Changing the setting must produce that exact format.
        if (artifactFormat != DXGI_FORMAT_UNKNOWN
            && metadata.format == artifactFormat
            && metadata.width % 4 == 0
            && metadata.height % 4 == 0)
        {
            return DirectX::SaveToDDSFile(
                source.GetImages(), source.GetImageCount(), metadata,
                DirectX::DDS_FLAGS_NONE, outputPath.c_str());
        }

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
        const bool useBlockCompression = compressionFormat != TextureCompressionFormat::Uncompressed;
        const size_t alignedWidth = useBlockCompression
            ? (sourceMetadata.width + 3) & ~size_t(3)
            : sourceMetadata.width;
        const size_t alignedHeight = useBlockCompression
            ? (sourceMetadata.height + 3) & ~size_t(3)
            : sourceMetadata.height;
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

        if (SUCCEEDED(hr) && compressionFormat == TextureCompressionFormat::Uncompressed)
        {
            hr = DirectX::SaveToDDSFile(
                importImages, importImageCount, importMetadata,
                DirectX::DDS_FLAGS_NONE, outputPath.c_str());
        }
        else
        {
            DirectX::ScratchImage compressed;
            if (SUCCEEDED(hr) && compressionFormat == TextureCompressionFormat::BC3)
            {
                hr = DirectX::Compress(
                    importImages, importImageCount, importMetadata,
                    artifactFormat,
                    static_cast<DirectX::TEX_COMPRESS_FLAGS>(
                        DirectX::TEX_COMPRESS_PARALLEL | DirectX::TEX_COMPRESS_DITHER),
                    DirectX::TEX_THRESHOLD_DEFAULT, compressed);
            }
            else if (SUCCEEDED(hr) && compressionFormat == TextureCompressionFormat::BC7)
            {
                // Search all BC7 modes, including the expensive three-subset modes.
                // The GPU path uses DirectCompute on the immediate context.
                const auto qualityFlags = DirectX::TEX_COMPRESS_BC7_USE_3SUBSETS;
                ComPtr<ID3D11Device> device = DEVICE;
                if (device != nullptr)
                {
                    hr = DirectX::Compress(
                        device.Get(), importImages, importImageCount, importMetadata,
                        artifactFormat, qualityFlags, 1.0f, compressed);
                }
                else
                {
                    hr = E_POINTER;
                }

                if (FAILED(hr))
                {
                    DBG->LogW(L"[TextureMeta] GPU BC7 compression failed; falling back to CPU: " + outputPath.wstring());
                    const auto cpuFlags = static_cast<DirectX::TEX_COMPRESS_FLAGS>(
                        DirectX::TEX_COMPRESS_PARALLEL | qualityFlags);
                    hr = DirectX::Compress(
                        importImages, importImageCount, importMetadata,
                        artifactFormat, cpuFlags, DirectX::TEX_THRESHOLD_DEFAULT, compressed);
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = DirectX::SaveToDDSFile(
                    compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(),
                    DirectX::DDS_FLAGS_NONE, outputPath.c_str());
            }
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
    hr = SaveArtifacts(
        source, metadata, outputPath, GetThumbnailPath(),
        _keepCpuPixels, &thumbnailHr, _compressionFormat);
    if (FAILED(hr))
        DBG->LogErrorW(L"[TextureMeta] Failed to build texture artifact: " + outputPath.wstring());
    if (FAILED(thumbnailHr))
        DBG->LogErrorW(L"[TextureMeta] Failed to build thumbnail artifact: " + GetThumbnailPath().wstring());
}
