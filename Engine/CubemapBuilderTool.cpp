#include "pch.h"
#include "CubemapBuilderTool.h"

#include "FileUtils.h"

namespace
{
    constexpr const char* FaceNames[] =
    {
        "Right (+X)",
        "Left (-X)",
        "Top (+Y)",
        "Bottom (-Y)",
        "Front (+Z)",
        "Back (-Z)",
    };

    HRESULT LoadImageFile(
        const fs::path& path,
        OUT DirectX::TexMetadata& metadata,
        OUT DirectX::ScratchImage& image)
    {
        wstring extension = path.extension().wstring();
        transform(extension.begin(), extension.end(), extension.begin(), towlower);
        if (extension == L".dds")
            return DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);
        if (extension == L".tga")
            return DirectX::LoadFromTGAFile(path.c_str(), &metadata, image);
        return DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);
    }

    string HResultMessage(const string& operation, HRESULT hr)
    {
        return Utils::Format("%s failed (HRESULT 0x%08X).", operation.c_str(), static_cast<unsigned int>(hr));
    }

    bool LoadFace(
        const fs::path& path,
        size_t expectedSize,
        OUT DirectX::ScratchImage& rgbaImage,
        OUT size_t& faceSize,
        OUT string& error)
    {
        DirectX::TexMetadata metadata = {};
        DirectX::ScratchImage loaded;
        HRESULT hr = LoadImageFile(path, OUT metadata, OUT loaded);
        if (FAILED(hr))
        {
            error = HResultMessage("Loading " + Utils::ToUtf8(path), hr);
            return false;
        }

        if (metadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D
            || metadata.arraySize != 1
            || metadata.width != metadata.height)
        {
            error = Utils::ToUtf8(path) + " must be one square 2D image.";
            return false;
        }
        if (expectedSize != 0 && metadata.width != expectedSize)
        {
            error = Utils::Format(
                "All six images must have the same size. Expected %zux%zu, but %s is %zux%zu.",
                expectedSize, expectedSize, Utils::ToUtf8(path).c_str(), metadata.width, metadata.height);
            return false;
        }

        const DirectX::Image* source = loaded.GetImage(0, 0, 0);
        DirectX::ScratchImage decompressed;
        if (source == nullptr)
        {
            error = "Image has no pixels: " + Utils::ToUtf8(path);
            return false;
        }
        if (DirectX::IsCompressed(source->format))
        {
            hr = DirectX::Decompress(*source, DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
            if (FAILED(hr))
            {
                error = HResultMessage("Decompressing " + Utils::ToUtf8(path), hr);
                return false;
            }
            source = decompressed.GetImage(0, 0, 0);
        }

        if (source->format == DXGI_FORMAT_R8G8B8A8_UNORM)
        {
            hr = rgbaImage.InitializeFromImage(*source);
        }
        else
        {
            hr = DirectX::Convert(
                *source, DXGI_FORMAT_R8G8B8A8_UNORM,
                DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgbaImage);
        }
        if (FAILED(hr))
        {
            error = HResultMessage("Converting " + Utils::ToUtf8(path), hr);
            return false;
        }

        faceSize = metadata.width;
        return true;
    }
}

CubemapBuilderTool::CubemapBuilderTool()
    : Super("Cubemap Builder")
{
    IsOpen = false;
}

bool CubemapBuilderTool::BuildCubemap(
    const array<fs::path, 6>& facePaths,
    const fs::path& outputPath,
    OUT string& error)
{
    error.clear();
    for (size_t face = 0; face < facePaths.size(); ++face)
    {
        if (facePaths[face].empty() || fs::is_regular_file(facePaths[face]) == false)
        {
            error = string("Select an image for ") + FaceNames[face] + ".";
            return false;
        }
    }

    wstring outputExtension = outputPath.extension().wstring();
    transform(outputExtension.begin(), outputExtension.end(), outputExtension.begin(), towlower);
    if (outputPath.empty() || outputExtension != L".dds")
    {
        error = "Choose an output file with the .dds extension.";
        return false;
    }

    array<DirectX::ScratchImage, 6> faces;
    size_t faceSize = 0;
    for (size_t face = 0; face < faces.size(); ++face)
    {
        size_t loadedSize = 0;
        if (LoadFace(facePaths[face], faceSize, OUT faces[face], OUT loadedSize, OUT error) == false)
            return false;
        if (faceSize == 0)
            faceSize = loadedSize;
    }

    DirectX::ScratchImage cubeBase;
    HRESULT hr = cubeBase.InitializeCube(DXGI_FORMAT_R8G8B8A8_UNORM, faceSize, faceSize, 1, 1);
    if (FAILED(hr))
    {
        error = HResultMessage("Allocating the cubemap", hr);
        return false;
    }

    for (size_t face = 0; face < faces.size(); ++face)
    {
        const DirectX::Image* source = faces[face].GetImage(0, 0, 0);
        const DirectX::Image* destination = cubeBase.GetImage(0, face, 0);
        if (source == nullptr || destination == nullptr)
        {
            error = string("Failed to access pixels for ") + FaceNames[face] + ".";
            return false;
        }

        for (size_t y = 0; y < faceSize; ++y)
        {
            memcpy(
                destination->pixels + y * destination->rowPitch,
                source->pixels + y * source->rowPitch,
                faceSize * 4);
        }
    }

    DirectX::ScratchImage mipChain;
    hr = DirectX::GenerateMipMaps(
        cubeBase.GetImages(), cubeBase.GetImageCount(), cubeBase.GetMetadata(),
        DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
    if (FAILED(hr))
    {
        error = HResultMessage("Generating mipmaps", hr);
        return false;
    }

    DirectX::ScratchImage compressed;
    hr = DirectX::Compress(
        mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(),
        DXGI_FORMAT_BC1_UNORM,
        static_cast<DirectX::TEX_COMPRESS_FLAGS>(
            DirectX::TEX_COMPRESS_PARALLEL | DirectX::TEX_COMPRESS_DITHER),
        DirectX::TEX_THRESHOLD_DEFAULT, compressed);
    if (FAILED(hr))
    {
        error = HResultMessage("Compressing to BC1", hr);
        return false;
    }

    std::error_code directoryError;
    if (outputPath.parent_path().empty() == false)
        fs::create_directories(outputPath.parent_path(), directoryError);
    if (directoryError)
    {
        error = "Failed to create the output directory: " + directoryError.message();
        return false;
    }

    hr = DirectX::SaveToDDSFile(
        compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(),
        DirectX::DDS_FLAGS_FORCE_DX9_LEGACY, outputPath.c_str());
    if (FAILED(hr))
    {
        error = HResultMessage("Saving the DDS cubemap", hr);
        return false;
    }
    return true;
}

void CubemapBuilderTool::OnGUI()
{
    ImGui::TextWrapped("Select six square images. Their size is detected automatically.");
    ImGui::Separator();

    for (size_t face = 0; face < _facePaths.size(); ++face)
    {
        ImGui::PushID(static_cast<int>(face));
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%-12s", FaceNames[face]);
        ImGui::SameLine(145.0f);
        if (ImGui::Button("Choose Image", ImVec2(110.0f, 0.0f)))
            SelectFace(face);
        ImGui::SameLine();
        const string fileName = _facePaths[face].empty()
            ? "(not selected)"
            : Utils::ToUtf8(_facePaths[face].filename());
        ImGui::TextUnformatted(fileName.c_str());
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Output DDS");
    ImGui::SameLine(145.0f);
    if (ImGui::Button("Choose Output", ImVec2(110.0f, 0.0f)))
        SelectOutput();
    ImGui::SameLine();
    const string outputName = _outputPath.empty()
        ? "(not selected)"
        : Utils::ToUtf8(_outputPath.filename());
    ImGui::TextUnformatted(outputName.c_str());

    ImGui::Spacing();
    if (ImGui::Button("Create DDS Cubemap", ImVec2(190.0f, 34.0f)))
        Build();

    if (_status.empty() == false)
    {
        ImGui::Spacing();
        const ImVec4 color = _lastBuildSucceeded
            ? ImVec4(0.35f, 0.9f, 0.45f, 1.0f)
            : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped("%s", _status.c_str());
        ImGui::PopStyleColor();
    }
}

void CubemapBuilderTool::SelectFace(size_t faceIndex)
{
    const fs::path initialDirectory = _facePaths[faceIndex].empty()
        ? fs::path(L"..\\Assets\\Textures\\Sky")
        : _facePaths[faceIndex].parent_path();
    const fs::path selected = FileUtils::OpenFileDialog(
        L"Select Cubemap Face",
        L"Image Files (*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.tif;*.dds)\0*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.tif;*.dds\0All Files (*.*)\0*.*\0",
        L"png",
        initialDirectory);
    if (selected.empty())
        return;

    _facePaths[faceIndex] = selected;
    if (_outputPath.empty())
        _outputPath = selected.parent_path() / (selected.stem().wstring() + L"_cube.dds");
    _status.clear();
}

void CubemapBuilderTool::SelectOutput()
{
    const fs::path initialDirectory = _outputPath.empty()
        ? fs::path(L"..\\Assets\\Textures\\Sky")
        : _outputPath.parent_path();
    const fs::path selected = FileUtils::SaveFileDialog(
        L"Save DDS Cubemap",
        L"DDS Cubemap (*.dds)\0*.dds\0All Files (*.*)\0*.*\0",
        L"dds",
        initialDirectory);
    if (selected.empty() == false)
    {
        _outputPath = selected;
        _status.clear();
    }
}

void CubemapBuilderTool::Build()
{
    string error;
    if (BuildCubemap(_facePaths, _outputPath, OUT error) == false)
    {
        SetStatus(false, error);
        DBG->LogError("[CubemapBuilderTool] " + error);
        return;
    }

    const string message = "Created cubemap: " + Utils::ToUtf8(_outputPath);
    SetStatus(true, message);
    DBG->Log("[CubemapBuilderTool] " + message);
}

void CubemapBuilderTool::SetStatus(bool succeeded, const string& message)
{
    _lastBuildSucceeded = succeeded;
    _status = message;
}
