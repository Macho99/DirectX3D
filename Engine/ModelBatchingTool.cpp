#include "pch.h"
#include "ModelBatchingTool.h"

#include "FileUtils.h"
#include "GameObject.h"
#include "Geometry.h"
#include "Material.h"
#include "MetaFile.h"
#include "Model.h"
#include "ModelMesh.h"
#include "ModelMeshResource.h"
#include "ModelRenderer.h"
#include "OnGUIUtils.h"
#include "Shader.h"
#include "Texture.h"
#include "Transform.h"

namespace
{
    using TimingClock = chrono::steady_clock;

    struct ModelMeshSizeEstimate
    {
        uint64 outputBytes = 0;
        size_t instanceCount = 0;
        size_t unavailableRendererCount = 0;
        bool overflowed = false;
    };

    string FormatFileSize(uint64 bytes)
    {
        static constexpr const char* Units[] = { "byte", "KB", "MB", "GB", "TB" };
        double value = static_cast<double>(bytes);
        size_t unitIndex = 0;
        while (value >= 1024.0 && unitIndex + 1 < _countof(Units))
        {
            value /= 1024.0;
            ++unitIndex;
        }

        if (unitIndex == 0)
            return Utils::Format("%llu %s", static_cast<unsigned long long>(bytes), Units[unitIndex]);
        return Utils::Format("%.2f %s", value, Units[unitIndex]);
    }

    void RefreshModelRendererSlot(OUT ModelRendererSlot& slot)
    {
        slot.sourceMeshBytes = 0;
        slot.estimatedOutputBytes = 0;
        slot.instanceCount = 0;
        slot.materialCount = 0;
        slot.estimateInitialized = true;
        slot.sizeAvailable = false;
        slot.materialCountAvailable = false;
        slot.overflowed = false;

        ModelRenderer* renderer = slot.rendererRef.Resolve();
        Model* model = renderer != nullptr ? renderer->GetModel().Resolve() : nullptr;
        if (model == nullptr)
            return;

        slot.materialCount = model->GetMaterialCount();
        slot.materialCountAvailable = true;

        ModelMeshResource* meshResource = model->GetMesh();
        if (meshResource == nullptr)
            return;

        MetaFile* meta = nullptr;
        if (RESOURCES->TryGetMetaByAssetId(meshResource->GetID(), OUT meta) == false
            || meta == nullptr)
        {
            return;
        }

        const fs::path meshPath = meta->GetImportedAssetPath(meshResource->GetID());
        std::error_code fileError;
        const uintmax_t fileBytes = fs::file_size(meshPath, fileError);
        if (fileError || fileBytes > UINT64_MAX)
            return;

        slot.sourceMeshBytes = static_cast<uint64>(fileBytes);
        slot.instanceCount = renderer->HasInstancingData()
            ? static_cast<size_t>(renderer->GetInstancingCount())
            : 1;
        slot.sizeAvailable = true;

        if (slot.instanceCount > 0
            && slot.sourceMeshBytes > UINT64_MAX / slot.instanceCount)
        {
            slot.estimatedOutputBytes = UINT64_MAX;
            slot.overflowed = true;
            return;
        }
        slot.estimatedOutputBytes = slot.sourceMeshBytes * slot.instanceCount;
    }

    void AccumulateModelMeshSizeEstimate(
        const ModelRendererSlot& slot,
        OUT ModelMeshSizeEstimate& total)
    {
        if (slot.sizeAvailable == false)
        {
            if (slot.rendererRef.IsValid())
                ++total.unavailableRendererCount;
            return;
        }

        total.instanceCount += slot.instanceCount;
        if (slot.overflowed
            || total.outputBytes > UINT64_MAX - slot.estimatedOutputBytes)
        {
            total.outputBytes = UINT64_MAX;
            total.overflowed = true;
            return;
        }
        total.outputBytes += slot.estimatedOutputBytes;
    }

    double ElapsedMilliseconds(const TimingClock::time_point& start)
    {
        return chrono::duration<double, std::milli>(TimingClock::now() - start).count();
    }

    void LogTiming(const string& stage, const TimingClock::time_point& start)
    {
        DBG->Log(Utils::Format(
            "[ModelBatchingTool][Timing] %s: %.2f ms",
            stage.c_str(), ElapsedMilliseconds(start)));
    }

    struct AtlasTiming
    {
        double allocateMs = 0.0;
        double fillMs = 0.0;
        double resolveMs = 0.0;
        double textureAccessMs = 0.0;
        double captureMs = 0.0;
        double decompressMs = 0.0;
        double convertMs = 0.0;
        double resizeMs = 0.0;
        double blitMs = 0.0;
        size_t textureCount = 0;
        size_t fallbackCount = 0;
    };

    void LogAtlasTiming(
        const char* atlasName,
        const AtlasTiming& timing,
        const TimingClock::time_point& totalStart,
        bool succeeded)
    {
        DBG->Log(Utils::Format(
            "[ModelBatchingTool][Timing][%s%s] total=%.2f ms | allocate=%.2f | fill=%.2f | resolve=%.2f | access=%.2f | GPU capture=%.2f | decompress=%.2f | convert=%.2f | resize=%.2f | blit=%.2f | textures=%zu | fallback=%zu",
            atlasName, succeeded ? "" : " FAILED",
            ElapsedMilliseconds(totalStart), timing.allocateMs, timing.fillMs,
            timing.resolveMs, timing.textureAccessMs, timing.captureMs,
            timing.decompressMs, timing.convertMs, timing.resizeMs, timing.blitMs,
            timing.textureCount, timing.fallbackCount));
    }

    struct MaterialSlot
    {
        wstring name;
        Material* material = nullptr;
    };

    wstring GetMaterialName(const ModelMesh& mesh, const Material& material)
    {
        if (material.GetName().empty() == false)
            return material.GetName();
        return mesh.materialName;
    }

    bool CollectMaterials(
        const vector<ModelRendererSlot>& rendererSlots,
        bool strict,
        OUT vector<MaterialSlot>& materials,
        OUT Shader*& commonShader,
        OUT string& error)
    {
        materials.clear();
        commonShader = nullptr;
        error.clear();

        unordered_map<wstring, size_t> slotsByName;
        size_t validRendererCount = 0;

        for (size_t rendererIndex = 0; rendererIndex < rendererSlots.size(); ++rendererIndex)
        {
            const ComponentRef<ModelRenderer>& rendererRef = rendererSlots[rendererIndex].rendererRef;
            ModelRenderer* renderer = rendererRef.Resolve();
            if (renderer == nullptr)
            {
                if (strict && rendererRef.IsValid())
                {
                    error = Utils::Format("Model Renderer %zu is missing.", rendererIndex);
                    return false;
                }
                continue;
            }

            ++validRendererCount;
            Model* model = renderer->GetModel().Resolve();
            if (model == nullptr)
            {
                if (strict)
                {
                    error = Utils::Format("Model Renderer %zu has no valid model.", rendererIndex);
                    return false;
                }
                continue;
            }

            ModelMeshResource* meshResource = model->GetMesh();
            if (meshResource == nullptr)
            {
                if (strict)
                {
                    error = Utils::Format("Model Renderer %zu has no valid model mesh.", rendererIndex);
                    return false;
                }
                continue;
            }

            for (const shared_ptr<ModelMesh>& mesh : meshResource->GetMeshes())
            {
                if (mesh == nullptr)
                    continue;

                Material* material = mesh->material.Resolve();
                if (material == nullptr)
                {
                    if (strict)
                    {
                        error = "A model sub-mesh has no valid material.";
                        return false;
                    }
                    continue;
                }

                Shader* shader = material->GetShader();
                if (shader == nullptr)
                {
                    if (strict)
                    {
                        error = "A material has no shader.";
                        return false;
                    }
                }
                else if (commonShader == nullptr)
                {
                    commonShader = shader;
                }
                else if (commonShader != shader && strict)
                {
                    error = "All materials used by the input models must use the same shader.";
                    return false;
                }

                const wstring materialName = GetMaterialName(*mesh, *material);
                if (materialName.empty())
                {
                    if (strict)
                    {
                        error = "A material has no name, so it cannot be assigned an atlas slot.";
                        return false;
                    }
                    continue;
                }

                if (slotsByName.find(materialName) == slotsByName.end())
                {
                    slotsByName[materialName] = materials.size();
                    materials.push_back({ materialName, material });
                }
            }
        }

        if (strict && validRendererCount == 0)
        {
            error = "Add at least one valid Model Renderer.";
            return false;
        }
        if (strict && materials.empty())
        {
            error = "No materials were found in the input models.";
            return false;
        }
        return true;
    }

    void FillImage(const DirectX::Image& image, uint8 red, uint8 green, uint8 blue, uint8 alpha)
    {
        for (size_t y = 0; y < image.height; ++y)
        {
            uint8* row = image.pixels + y * image.rowPitch;
            for (size_t x = 0; x < image.width; ++x)
            {
                uint8* pixel = row + x * 4;
                pixel[0] = red;
                pixel[1] = green;
                pixel[2] = blue;
                pixel[3] = alpha;
            }
        }
    }

    HRESULT ResizeRgba8(
        const DirectX::Image& source,
        size_t destinationSize,
        OUT DirectX::ScratchImage& output)
    {
        if ((source.format != DXGI_FORMAT_R8G8B8A8_UNORM
            && source.format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
            || source.width == 0 || source.height == 0 || destinationSize == 0)
        {
            return E_INVALIDARG;
        }

        HRESULT hr = output.Initialize2D(
            source.format, destinationSize, destinationSize, 1, 1);
        if (FAILED(hr))
            return hr;

        const DirectX::Image* destination = output.GetImage(0, 0, 0);
        if (destination == nullptr)
            return E_FAIL;

        const double scaleX = static_cast<double>(source.width) / destinationSize;
        const double scaleY = static_cast<double>(source.height) / destinationSize;
        for (size_t y = 0; y < destinationSize; ++y)
        {
            const double sourceY = (static_cast<double>(y) + 0.5) * scaleY - 0.5;
            const int64 y0Unclamped = static_cast<int64>(floor(sourceY));
            const int64 y1Unclamped = y0Unclamped + 1;
            const size_t y0 = static_cast<size_t>(std::clamp<int64>(y0Unclamped, 0, source.height - 1));
            const size_t y1 = static_cast<size_t>(std::clamp<int64>(y1Unclamped, 0, source.height - 1));
            const float blendY = static_cast<float>(sourceY - floor(sourceY));
            const uint8* sourceRow0 = source.pixels + y0 * source.rowPitch;
            const uint8* sourceRow1 = source.pixels + y1 * source.rowPitch;
            uint8* destinationRow = destination->pixels + y * destination->rowPitch;

            for (size_t x = 0; x < destinationSize; ++x)
            {
                const double sourceX = (static_cast<double>(x) + 0.5) * scaleX - 0.5;
                const int64 x0Unclamped = static_cast<int64>(floor(sourceX));
                const int64 x1Unclamped = x0Unclamped + 1;
                const size_t x0 = static_cast<size_t>(std::clamp<int64>(x0Unclamped, 0, source.width - 1));
                const size_t x1 = static_cast<size_t>(std::clamp<int64>(x1Unclamped, 0, source.width - 1));
                const float blendX = static_cast<float>(sourceX - floor(sourceX));

                const uint8* topLeft = sourceRow0 + x0 * 4;
                const uint8* topRight = sourceRow0 + x1 * 4;
                const uint8* bottomLeft = sourceRow1 + x0 * 4;
                const uint8* bottomRight = sourceRow1 + x1 * 4;
                uint8* destinationPixel = destinationRow + x * 4;
                for (size_t channel = 0; channel < 4; ++channel)
                {
                    const float top = topLeft[channel]
                        + (topRight[channel] - topLeft[channel]) * blendX;
                    const float bottom = bottomLeft[channel]
                        + (bottomRight[channel] - bottomLeft[channel]) * blendX;
                    destinationPixel[channel] = static_cast<uint8>(
                        std::clamp(top + (bottom - top) * blendY, 0.0f, 255.0f) + 0.5f);
                }
            }
        }
        return S_OK;
    }

    bool PrepareTextureTile(
        Texture* texture,
        size_t tileSize,
        DXGI_FORMAT outputFormat,
        AtlasTiming& timing,
        OUT DirectX::ScratchImage& output,
        OUT string& error)
    {
        if (texture == nullptr)
            return true;

        TimingClock::time_point stageStart = TimingClock::now();
        ComPtr<ID3D11Texture2D> texture2D = texture->GetTexture2D();
        timing.textureAccessMs += ElapsedMilliseconds(stageStart);
        if (texture2D == nullptr)
        {
            error = "Failed to access a source texture.";
            return false;
        }

        DirectX::ScratchImage captured;
        stageStart = TimingClock::now();
        HRESULT hr = DirectX::CaptureTexture(DEVICE.Get(), DC.Get(), texture2D.Get(), captured);
        timing.captureMs += ElapsedMilliseconds(stageStart);
        if (FAILED(hr))
        {
            error = Utils::Format("Failed to capture a source texture (HRESULT 0x%08X).", static_cast<uint32>(hr));
            return false;
        }

        const DirectX::Image* source = captured.GetImage(0, 0, 0);
        DirectX::ScratchImage decompressed;
        if (source != nullptr && DirectX::IsCompressed(source->format))
        {
            stageStart = TimingClock::now();
            hr = DirectX::Decompress(*source, DXGI_FORMAT_UNKNOWN, decompressed);
            timing.decompressMs += ElapsedMilliseconds(stageStart);
            if (FAILED(hr))
            {
                error = Utils::Format("Failed to decompress a source texture (HRESULT 0x%08X).", static_cast<uint32>(hr));
                return false;
            }
            source = decompressed.GetImage(0, 0, 0);
        }

        if (source == nullptr)
        {
            error = "A source texture contains no image data.";
            return false;
        }

        DirectX::ScratchImage converted;
        if (source->format != outputFormat)
        {
            stageStart = TimingClock::now();
            hr = DirectX::Convert(
                *source, outputFormat, DirectX::TEX_FILTER_DEFAULT,
                DirectX::TEX_THRESHOLD_DEFAULT, converted);
            timing.convertMs += ElapsedMilliseconds(stageStart);
            if (FAILED(hr))
            {
                error = Utils::Format("Failed to convert a source texture (HRESULT 0x%08X).", static_cast<uint32>(hr));
                return false;
            }
            source = converted.GetImage(0, 0, 0);
        }

        stageStart = TimingClock::now();
        if (source->width == tileSize && source->height == tileSize)
        {
            hr = output.InitializeFromImage(*source);
        }
        else
        {
            hr = ResizeRgba8(*source, tileSize, OUT output);
        }
        timing.resizeMs += ElapsedMilliseconds(stageStart);

        if (FAILED(hr))
        {
            error = Utils::Format("Failed to resize a source texture (HRESULT 0x%08X).", static_cast<uint32>(hr));
            return false;
        }
        return true;
    }

    using TextureSelector = ResourceRef<Texture>(Material::*)();

    bool BuildAtlas(
        const char* atlasName,
        const vector<MaterialSlot>& materials,
        size_t gridSize,
        size_t tileSize,
        DXGI_FORMAT format,
        TextureSelector selector,
        const Color& fallback,
        OUT DirectX::ScratchImage& atlas,
        OUT string& error)
    {
        const TimingClock::time_point totalStart = TimingClock::now();
        AtlasTiming timing;
        const size_t atlasSize = gridSize * tileSize;
        TimingClock::time_point stageStart = TimingClock::now();
        HRESULT hr = atlas.Initialize2D(format, atlasSize, atlasSize, 1, 1);
        timing.allocateMs = ElapsedMilliseconds(stageStart);
        if (FAILED(hr))
        {
            error = Utils::Format("Failed to allocate an atlas (HRESULT 0x%08X).", static_cast<uint32>(hr));
            LogAtlasTiming(atlasName, timing, totalStart, false);
            return false;
        }

        const DirectX::Image* atlasImage = atlas.GetImage(0, 0, 0);
        const auto toByte = [](float value)
            {
                return static_cast<uint8>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
            };
        stageStart = TimingClock::now();
        FillImage(*atlasImage, toByte(fallback.x), toByte(fallback.y), toByte(fallback.z), toByte(fallback.w));
        timing.fillMs = ElapsedMilliseconds(stageStart);

        for (size_t slot = 0; slot < materials.size(); ++slot)
        {
            const TimingClock::time_point textureStart = TimingClock::now();
            ResourceRef<Texture> textureRef = (materials[slot].material->*selector)();
            stageStart = TimingClock::now();
            Texture* texture = textureRef.Resolve();
            timing.resolveMs += ElapsedMilliseconds(stageStart);
            if (texture == nullptr)
            {
                ++timing.fallbackCount;
                continue;
            }
            ++timing.textureCount;

            DirectX::ScratchImage tile;
            if (PrepareTextureTile(texture, tileSize, format, timing, OUT tile, OUT error) == false)
            {
                error = "Material '" + Utils::ToString(materials[slot].name) + "': " + error;
                LogAtlasTiming(atlasName, timing, totalStart, false);
                return false;
            }

            const DirectX::Image* tileImage = tile.GetImage(0, 0, 0);
            const size_t dstX = (slot % gridSize) * tileSize;
            const size_t dstY = (slot / gridSize) * tileSize;
            stageStart = TimingClock::now();
            for (size_t y = 0; y < tileSize; ++y)
            {
                const uint8* sourceRow = tileImage->pixels + y * tileImage->rowPitch;
                uint8* destinationRow = atlasImage->pixels + (dstY + y) * atlasImage->rowPitch + dstX * 4;
                memcpy(destinationRow, sourceRow, tileSize * 4);
            }
            timing.blitMs += ElapsedMilliseconds(stageStart);

            const double textureMs = ElapsedMilliseconds(textureStart);
            if (textureMs >= 50.0)
            {
                DBG->Log(Utils::Format(
                    "[ModelBatchingTool][Timing][%s] slow material '%s': %.2f ms",
                    atlasName, Utils::ToString(materials[slot].name).c_str(), textureMs));
            }
        }
        LogAtlasTiming(atlasName, timing, totalStart, true);
        return true;
    }

    bool SaveAtlas(const DirectX::ScratchImage& atlas, const fs::path& path, OUT string& error)
    {
        const DirectX::Image* image = atlas.GetImage(0, 0, 0);
        if (image == nullptr)
        {
            error = "An atlas contains no image data.";
            return false;
        }

        const HRESULT hr = DirectX::SaveToWICFile(
            *image, DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), path.c_str());
        if (FAILED(hr))
        {
            error = Utils::Format("Failed to save '%s' (HRESULT 0x%08X).", path.string().c_str(), static_cast<uint32>(hr));
            return false;
        }
        return true;
    }

    bool SaveModelMeshBinary(
        const fs::path& path,
        const string& meshName,
        const string& materialName,
        Geometry<ModelVertexType>& geometry,
        OUT string& error)
    {
        const vector<ModelVertexType>& vertices = geometry.GetVertices();
        const vector<uint32>& indices = geometry.GetIndices();
        if (vertices.size() > UINT32_MAX || indices.size() > UINT32_MAX
            || vertices.size() > UINT32_MAX / sizeof(ModelVertexType)
            || indices.size() > UINT32_MAX / sizeof(uint32))
        {
            error = "The batched model mesh is too large for the binary model-mesh format.";
            return false;
        }

        FileUtils file;
        file.Open(path.wstring(), FileMode::Write);

        // Converter::WriteModelFile / ModelMeshResource::ReadModel layout.
        file.Write<uint32>(0); // Bone count. All transforms are already baked.
        file.Write<uint32>(1); // One combined sub-mesh.
        file.Write(meshName);
        file.Write<int32>(0);
        file.Write(materialName);

        file.Write<uint32>(static_cast<uint32>(vertices.size()));
        if (vertices.empty() == false)
        {
            file.Write(
                vertices.data(),
                static_cast<uint32>(sizeof(ModelVertexType) * vertices.size()));
        }

        file.Write<uint32>(static_cast<uint32>(indices.size()));
        if (indices.empty() == false)
        {
            file.Write(
                indices.data(),
                static_cast<uint32>(sizeof(uint32) * indices.size()));
        }
        return true;
    }

    float WrapUv(float value)
    {
        if (value >= 0.0f && value <= 1.0f)
            return value;
        return value - floorf(value);
    }

    bool AppendBakedMesh(
        const ModelMesh& sourceMesh,
        const Matrix& worldMatrix,
        size_t materialSlot,
        size_t gridSize,
        size_t tileSize,
        OUT Geometry<ModelVertexType>& destination,
        OUT string& error)
    {
        if (sourceMesh.geometry == nullptr)
            return true;

        const vector<ModelVertexType>& sourceVertices = sourceMesh.geometry->GetVertices();
        const vector<uint32>& sourceIndices = sourceMesh.geometry->GetIndices();
        if (sourceVertices.size() > UINT32_MAX
            || destination.GetVertexCount() > UINT32_MAX - static_cast<uint32>(sourceVertices.size()))
        {
            error = "The batched mesh exceeds the 32-bit vertex index limit.";
            return false;
        }

        const Matrix boneMatrix = sourceMesh.bone != nullptr
            ? sourceMesh.bone->globalMatrix
            : Matrix::Identity;
        const Matrix finalMatrix = boneMatrix * worldMatrix;
        const Matrix normalMatrix = finalMatrix.Invert().Transpose();
        const uint32 baseVertex = destination.GetVertexCount();
        const float atlasSize = static_cast<float>(gridSize * tileSize);
        const float tileSpan = static_cast<float>(tileSize > 1 ? tileSize - 1 : 0);
        const float tileX = static_cast<float>((materialSlot % gridSize) * tileSize) + 0.5f;
        const float tileY = static_cast<float>((materialSlot / gridSize) * tileSize) + 0.5f;

        for (const ModelVertexType& sourceVertex : sourceVertices)
        {
            ModelVertexType destinationVertex;
            destinationVertex.position = Vec3::Transform(sourceVertex.position, finalMatrix);

            destinationVertex.normal = Vec3::TransformNormal(sourceVertex.normal, normalMatrix);
            if (destinationVertex.normal.LengthSquared() > 0.000001f)
                destinationVertex.normal.Normalize();

            destinationVertex.tangent = Vec3::TransformNormal(sourceVertex.tangent, finalMatrix);
            if (destinationVertex.tangent.LengthSquared() > 0.000001f)
                destinationVertex.tangent.Normalize();

            const float sourceU = WrapUv(sourceVertex.uv.x);
            const float sourceV = WrapUv(sourceVertex.uv.y);
            destinationVertex.uv.x = (tileX + sourceU * tileSpan) / atlasSize;
            destinationVertex.uv.y = (tileY + sourceV * tileSpan) / atlasSize;
            destination.AddVertex(destinationVertex);
        }

        const bool mirrored = finalMatrix.Determinant() < 0.0f;
        if (mirrored && sourceIndices.size() % 3 == 0)
        {
            for (size_t index = 0; index < sourceIndices.size(); index += 3)
            {
                destination.AddIndex(baseVertex + sourceIndices[index]);
                destination.AddIndex(baseVertex + sourceIndices[index + 2]);
                destination.AddIndex(baseVertex + sourceIndices[index + 1]);
            }
        }
        else
        {
            for (uint32 index : sourceIndices)
                destination.AddIndex(baseVertex + index);
        }
        return true;
    }
}

ModelBatchingTool::ModelBatchingTool()
    : Super("Model Batching Tool", false)
{
    IsOpen = false;
}

void ModelBatchingTool::OnGUI()
{
    ImGui::SeparatorText("Input Model Renderers");
    OnGUIUtils::DrawComponentRef("Extract Root", _autoExtractRoot);

    const bool canAutoExtract = _autoExtractRoot.Resolve() != nullptr;
    ImGui::BeginDisabled(canAutoExtract == false);
    if (ImGui::Button("Auto Extract"))
        ExtractModelRenderersFromChildren();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Remove All"))
    {
        _modelRenderers.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Sort"))
    {
        std::sort(_modelRenderers.begin(), _modelRenderers.end(),
            [](const ModelRendererSlot& a, const ModelRendererSlot& b)
            {
                return a.estimatedOutputBytes > b.estimatedOutputBytes;
            });
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Register Model Renderers found in every descendant of Extract Root.");

    ModelMeshSizeEstimate meshSizeEstimate;
    for (size_t index = 0; index < _modelRenderers.size();)
    {
        ImGui::PushID(static_cast<int>(index));
        const bool remove = ImGui::SmallButton("Remove");
        ImGui::SameLine();
        ModelRendererSlot& rendererSlot = _modelRenderers[index];
        const bool rendererChanged =
            OnGUIUtils::DrawComponentRef(to_string(index).c_str(), rendererSlot.rendererRef);
        if (rendererChanged || rendererSlot.estimateInitialized == false)
            RefreshModelRendererSlot(OUT rendererSlot);

        if (rendererSlot.sizeAvailable)
        {
            const string formattedSourceSize = FormatFileSize(rendererSlot.sourceMeshBytes);
            const string formattedSize = FormatFileSize(rendererSlot.estimatedOutputBytes);
            ImGui::TextDisabled(
                "~%s x %zu = %s | Materials: %zu",
                formattedSourceSize.c_str(), rendererSlot.instanceCount, formattedSize.c_str(), rendererSlot.materialCount);
        }
        else if (rendererSlot.materialCountAvailable)
        {
            ImGui::TextDisabled(
                "Size unavailable | Materials: %zu",
                rendererSlot.materialCount);
        }
        else
        {
            ImGui::TextDisabled("Size: -- | Materials: --");
        }
        ImGui::PopID();

        if (remove)
            _modelRenderers.erase(_modelRenderers.begin() + index);
        else
        {
            AccumulateModelMeshSizeEstimate(rendererSlot, OUT meshSizeEstimate);
            ++index;
        }
    }

    if (ImGui::Button("Add Model Renderer"))
        _modelRenderers.emplace_back();

    if (meshSizeEstimate.instanceCount > 0)
    {
        const string formattedSize = FormatFileSize(meshSizeEstimate.outputBytes);
        const char* partialSuffix = meshSizeEstimate.unavailableRendererCount > 0
            ? " (partial estimate)"
            : "";
        ImGui::Text(
            "Estimated Output ModelMesh: ~%s (%zu instances)%s",
            formattedSize.c_str(), meshSizeEstimate.instanceCount, partialSuffix);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Sum of each input .modelmesh file size multiplied by that renderer's instance count.\n"
                "A renderer without instancing data counts as one instance. Atlas textures are not included.%s",
                meshSizeEstimate.overflowed
                    ? "\nThe estimate exceeded the supported display range."
                    : "");
        }
    }
    else if (meshSizeEstimate.unavailableRendererCount > 0)
    {
        ImGui::TextDisabled("Estimated Output ModelMesh: unavailable");
    }

    ImGui::SeparatorText("Atlas Settings");
    if (ImGui::InputInt("Max Atlas Size", &_maxAtlasSize))
        _maxAtlasSize = std::clamp(_maxAtlasSize, 1, 16384);
    if (ImGui::InputInt("Max Texture Size / Material", &_maxTextureSize))
        _maxTextureSize = std::clamp(_maxTextureSize, 1, 16384);

    const int cellsPerAxis = _maxTextureSize <= _maxAtlasSize
        ? _maxAtlasSize / _maxTextureSize
        : 0;
    const int64 maxMaterialCount = static_cast<int64>(cellsPerAxis) * cellsPerAxis;
    const size_t currentMaterialCount = GetCurrentMaterialCount();

    ImGui::Text("Grid: %d x %d", cellsPerAxis, cellsPerAxis);
    ImGui::Text("Materials: %zu / %lld", currentMaterialCount, maxMaterialCount);
    if (currentMaterialCount > 0 && cellsPerAxis > 0)
    {
        const int usedGrid = static_cast<int>(ceil(sqrt(static_cast<double>(currentMaterialCount))));
        ImGui::Text("Output Atlas: %d x %d", usedGrid * _maxTextureSize, usedGrid * _maxTextureSize);
    }
    if (currentMaterialCount > static_cast<size_t>(maxMaterialCount))
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "Too many materials for this atlas.");

    ImGui::Separator();
    const bool canBuild = currentMaterialCount > 0
        && maxMaterialCount > 0
        && currentMaterialCount <= static_cast<size_t>(maxMaterialCount);
    ImGui::BeginDisabled(canBuild == false);
    if (ImGui::Button("Build ModelMesh + Atlases"))
        Build();
    ImGui::EndDisabled();

    if (_status.empty() == false)
    {
        const ImVec4 color = _lastBuildSucceeded
            ? ImVec4(0.35f, 0.9f, 0.45f, 1.0f)
            : ImVec4(1.0f, 0.35f, 0.25f, 1.0f);
        ImGui::TextColored(color, "%s", _status.c_str());
    }
}

size_t ModelBatchingTool::ExtractModelRenderersFromChildren()
{
    Transform* root = _autoExtractRoot.Resolve();
    if (root == nullptr)
    {
        SetStatus(false, "Select a valid Extract Root.");
        return 0;
    }

    GuidRefSet registeredRenderers;
    for (const ModelRendererSlot& slot : _modelRenderers)
    {
        if (slot.rendererRef.IsValid())
            registeredRenderers.insert(slot.rendererRef);
    }

    size_t registeredCount = 0;
    function<void(Transform*)> visitDescendants = [&](Transform* transform)
        {
            if (transform == nullptr)
                return;

            GameObject* gameObject = transform->GetGameObject();
            if (gameObject != nullptr)
            {
                ComponentRef<ModelRenderer> rendererRef =
                    gameObject->GetFixedComponentRef<ModelRenderer>();
                if (rendererRef.Resolve() != nullptr
                    && registeredRenderers.insert(rendererRef).second)
                {
                    ModelRendererSlot* destination = nullptr;
                    for (ModelRendererSlot& slot : _modelRenderers)
                    {
                        if (slot.rendererRef.IsValid() == false)
                        {
                            destination = &slot;
                            break;
                        }
                    }

                    if (destination == nullptr)
                    {
                        _modelRenderers.emplace_back();
                        destination = &_modelRenderers.back();
                    }

                    destination->rendererRef = rendererRef;
                    RefreshModelRendererSlot(OUT *destination);
                    ++registeredCount;
                }
            }

            for (TransformRef& childRef : transform->GetChildren())
                visitDescendants(childRef.Resolve());
        };

    for (TransformRef& childRef : root->GetChildren())
        visitDescendants(childRef.Resolve());

    SetStatus(true, Utils::Format(
        "Auto Extract registered %zu Model Renderer%s.",
        registeredCount, registeredCount == 1 ? "" : "s"));
    return registeredCount;
}

bool ModelBatchingTool::Build()
{
    const TimingClock::time_point validationStart = TimingClock::now();
    vector<MaterialSlot> materials;
    Shader* commonShader = nullptr;
    string error;
    if (CollectMaterials(_modelRenderers, true, OUT materials, OUT commonShader, OUT error) == false)
    {
        LogTiming("Input validation (failed)", validationStart);
        SetStatus(false, error);
        return false;
    }
    LogTiming("Input validation", validationStart);

    const size_t cellsPerAxis = static_cast<size_t>(_maxAtlasSize / _maxTextureSize);
    const size_t capacity = cellsPerAxis * cellsPerAxis;
    if (cellsPerAxis == 0 || materials.size() > capacity)
    {
        SetStatus(false, "The material count exceeds the selected atlas capacity.");
        return false;
    }

    fs::path meshPath = FileUtils::SaveFileDialog(
        L"Save Batched Model Mesh",
        L"Model Mesh Files (*.modelmesh)\0*.modelmesh\0All Files (*.*)\0*.*\0",
        L"modelmesh",
        L"..\\Assets");
    if (meshPath.empty())
        return false;
    meshPath.replace_extension(ModelMeshResource::GetExtension());

    const TimingClock::time_point totalStart = TimingClock::now();
    const size_t gridSize = static_cast<size_t>(ceil(sqrt(static_cast<double>(materials.size()))));
    DBG->Log(Utils::Format(
        "[ModelBatchingTool][Timing] Build start | materials=%zu | grid=%zux%zu | tile=%d | atlas=%zux%zu",
        materials.size(), gridSize, gridSize, _maxTextureSize,
        gridSize * static_cast<size_t>(_maxTextureSize),
        gridSize * static_cast<size_t>(_maxTextureSize)));

    DirectX::ScratchImage diffuseAtlas;
    DirectX::ScratchImage normalAtlas;
    DirectX::ScratchImage specularAtlas;

    bool buildDiffuse = BuildAtlas(
        "Diffuse",
        materials, gridSize, static_cast<size_t>(_maxTextureSize),
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, &Material::GetDiffuseMap,
        Color(1.f, 1.f, 1.f, 1.f), OUT diffuseAtlas, OUT error);

    if (buildDiffuse == false)
    {
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }

    bool buildNormal = BuildAtlas(
        "Normal",
        materials, gridSize, static_cast<size_t>(_maxTextureSize),
        DXGI_FORMAT_R8G8B8A8_UNORM, &Material::GetNormalMap,
        Color(0.5f, 0.5f, 1.f, 1.f), OUT normalAtlas, OUT error);

    if (buildNormal == false)
    {
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }

    bool buildSpecular = BuildAtlas(
        "Specular",
        materials, gridSize, static_cast<size_t>(_maxTextureSize),
        DXGI_FORMAT_R8G8B8A8_UNORM, &Material::GetSpecularMap,
        Color(1.f, 1.f, 1.f, 1.f), OUT specularAtlas, OUT error);

    if (buildSpecular == false)
    {
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }

    unordered_map<wstring, size_t> slotsByName;
    for (size_t slot = 0; slot < materials.size(); ++slot)
        slotsByName[materials[slot].name] = slot;

    shared_ptr<Geometry<ModelVertexType>> geometry =
        make_shared<Geometry<ModelVertexType>>();
    const TimingClock::time_point meshBakeStart = TimingClock::now();
    size_t instanceCount = 0;
    for (const ModelRendererSlot& rendererSlot : _modelRenderers)
    {
        const ComponentRef<ModelRenderer>& rendererRef = rendererSlot.rendererRef;
        ModelRenderer* renderer = rendererRef.Resolve();
        if (renderer == nullptr)
            continue;

        vector<Matrix> worldMatrices;
        if (renderer->HasInstancingData())
        {
            const vector<InstancingData>& instancingDatas = renderer->GetInstancingDatas();
            if (instancingDatas.empty())
            {
                LogTiming("Mesh bake (failed)", meshBakeStart);
                LogTiming("Build total before failure", totalStart);
                SetStatus(false, "A renderer has instancing data, but its world matrices have not been updated yet.");
                return false;
            }
            worldMatrices.assign(instancingDatas.begin(), instancingDatas.end());
        }
        else
        {
            worldMatrices.push_back(renderer->GetTransform()->GetWorldMatrix());
        }
        instanceCount += worldMatrices.size();

        Model* model = renderer->GetModel().Resolve();
        ModelMeshResource* meshResource = model->GetMesh();
        for (const shared_ptr<ModelMesh>& mesh : meshResource->GetMeshes())
        {
            if (mesh == nullptr)
                continue;

            Material* material = mesh->material.Resolve();
            const wstring materialName = GetMaterialName(*mesh, *material);
            const auto slotIt = slotsByName.find(materialName);
            if (slotIt == slotsByName.end())
            {
                LogTiming("Mesh bake (failed)", meshBakeStart);
                LogTiming("Build total before failure", totalStart);
                SetStatus(false, "Failed to find an atlas slot for a material.");
                return false;
            }

            for (const Matrix& worldMatrix : worldMatrices)
            {
                if (AppendBakedMesh(
                    *mesh, worldMatrix, slotIt->second, gridSize,
                    static_cast<size_t>(_maxTextureSize), OUT *geometry, OUT error) == false)
                {
                    LogTiming("Mesh bake (failed)", meshBakeStart);
                    LogTiming("Build total before failure", totalStart);
                    SetStatus(false, error);
                    return false;
                }
            }
        }
    }
    DBG->Log(Utils::Format(
        "[ModelBatchingTool][Timing] Mesh bake: %.2f ms | instances=%zu | vertices=%u | indices=%u",
        ElapsedMilliseconds(meshBakeStart), instanceCount,
        geometry->GetVertexCount(), geometry->GetIndexCount()));

    const fs::path outputFolder = meshPath.parent_path();
    const wstring outputStem = meshPath.stem().wstring();
    const fs::path diffusePath = outputFolder / (outputStem + L"_Diffuse.png");
    const fs::path normalPath = outputFolder / (outputStem + L"_Normal.png");
    const fs::path specularPath = outputFolder / (outputStem + L"_Specular.png");
    TimingClock::time_point saveStart = TimingClock::now();
    if (SaveAtlas(diffuseAtlas, diffusePath, OUT error) == false)
    {
        LogTiming("Save Diffuse PNG (failed)", saveStart);
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }
    LogTiming("Save Diffuse PNG", saveStart);

    saveStart = TimingClock::now();
    if (SaveAtlas(normalAtlas, normalPath, OUT error) == false)
    {
        LogTiming("Save Normal PNG (failed)", saveStart);
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }
    LogTiming("Save Normal PNG", saveStart);

    saveStart = TimingClock::now();
    if (SaveAtlas(specularAtlas, specularPath, OUT error) == false)
    {
        LogTiming("Save Specular PNG (failed)", saveStart);
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }
    LogTiming("Save Specular PNG", saveStart);

    const TimingClock::time_point meshSaveStart = TimingClock::now();
    if (SaveModelMeshBinary(
        meshPath, Utils::ToString(outputStem), Utils::ToString(outputStem),
        *geometry, OUT error) == false)
    {
        LogTiming("Save ModelMesh binary (failed)", meshSaveStart);
        LogTiming("Build total before failure", totalStart);
        SetStatus(false, error);
        return false;
    }
    LogTiming("Save ModelMesh binary", meshSaveStart);
    LogTiming("Build total", totalStart);

    SetStatus(true, Utils::Format(
        "Built %zu materials and %zu instances. Saved '%s' and three atlas textures.",
        materials.size(), instanceCount, meshPath.string().c_str()));
    return true;
}

size_t ModelBatchingTool::GetCurrentMaterialCount() const
{
    vector<MaterialSlot> materials;
    Shader* commonShader = nullptr;
    string error;
    CollectMaterials(_modelRenderers, false, OUT materials, OUT commonShader, OUT error);
    return materials.size();
}

void ModelBatchingTool::SetStatus(bool succeeded, const string& message)
{
    _lastBuildSucceeded = succeeded;
    _status = message;
    if (succeeded)
        DBG->Log("[ModelBatchingTool] " + message);
    else
        DBG->LogError("[ModelBatchingTool] " + message);
}
