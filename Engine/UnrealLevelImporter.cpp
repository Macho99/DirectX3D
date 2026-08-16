#include "pch.h"
#include "UnrealLevelImporter.h"
#include "BatchInfo.h"
#include <cctype>
#include <fstream>
#include "Model.h"
#include "ModelRenderer.h"
#include <queue>

namespace
{
    string ToLower(string value)
    {
        transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });

        return value;
    }

    bool IsTreeMesh(const LevelMeshData& meshData)
    {
        return ToLower(meshData.MeshName).find("tree") != string::npos
            || ToLower(meshData.MeshPath).find("tree") != string::npos;
    }

    size_t LoadBatchedModelNames(
        const fs::path& rootPath,
        OUT unordered_set<string>& batchedModelNames)
    {
        batchedModelNames.clear();

        error_code error;
        if (!fs::is_directory(rootPath, error))
            return 0;

        size_t loadedFileCount = 0;
        const fs::directory_options options = fs::directory_options::skip_permission_denied;
        fs::recursive_directory_iterator iterator(rootPath, options, error);
        const fs::recursive_directory_iterator end;

        while (!error && iterator != end)
        {
            const fs::directory_entry& entry = *iterator;
            if (entry.is_regular_file(error) && !error
                && ToLower(entry.path().extension().string()) == ".batchinfo")
            {
                try
                {
                    std::ifstream file(entry.path());
                    if (!file.is_open())
                        throw std::runtime_error("failed to open file");

                    BatchInfo batchInfo;
                    cereal::JSONInputArchive archive(file);
                    archive(cereal::make_nvp("BatchInfo", batchInfo));
                    for (const string& modelName : batchInfo.ModelNames)
                    {
                        if (modelName.empty() == false)
                            batchedModelNames.insert(ToLower(modelName));
                    }
                    ++loadedFileCount;
                }
                catch (const std::exception& exception)
                {
                    DBG->LogWarning(Utils::Format(
                        "[UnrealLevelImporter] Failed to read Batch Info '%s': %s",
                        entry.path().string().c_str(), exception.what()));
                }
            }

            error.clear();
            iterator.increment(error);
        }

        return loadedFileCount;
    }
}

bool UnrealLevelImporter::LoadLevel(const string & path)
{
    LevelData outLevel;
    std::ifstream file(path);

    if (!file.is_open())
        return false;

    try
    {
        cereal::JSONInputArchive archive(file);

        archive(
            cereal::make_nvp("LevelName", outLevel.LevelName),
            cereal::make_nvp("MeshCount", outLevel.MeshCount),
            cereal::make_nvp("Meshes", outLevel.Meshes)
        );
    }
    catch (const std::exception&)
    {
        return false;
    }

    GameObject* rootObj = CUR_SCENE->Add(outLevel.LevelName).Resolve();
    Transform* rootTransform = rootObj->GetTransform();
    const fs::path modelRootPath = "..\\Assets\\Imports\\Rural_Cabin";
    const fs::path batchInfoRootPath = "..\\Assets\\Batches";
    unordered_set<string> batchedModelNames;
    const size_t batchInfoFileCount =
        LoadBatchedModelNames(batchInfoRootPath, OUT batchedModelNames);
    if (batchInfoFileCount > 0)
    {
        DBG->Log(Utils::Format(
            "[UnrealLevelImporter] Loaded %zu Batch Info file(s); excluding %zu source model(s).",
            batchInfoFileCount, batchedModelNames.size()));
    }
    unordered_map<string, Transform*> folderCache;
    unordered_map<string, pair<Transform*, ResourceRef<Model>>> modelCache;
    size_t skippedBatchedInstanceCount = 0;
    size_t individualTreeRendererCount = 0;

    for (int i = 0; i < 10000 && i < outLevel.MeshCount; i++)
    {
        const LevelMeshData& levelMeshData = outLevel.Meshes[i];
        const bool useIndividualTreeRenderer = IsTreeMesh(levelMeshData);
        if (batchedModelNames.find(ToLower(levelMeshData.MeshName))
            != batchedModelNames.end())
        {
            ++skippedBatchedInstanceCount;
            continue;
        }

        ResourceRef<Model> modelRef;
        Transform* meshParent = nullptr;

        auto it = modelCache.find(levelMeshData.MeshName);
        if (it == modelCache.end())
        {
            fs::path modelPath;
            if (TryFindModelPath(modelRootPath, levelMeshData.MeshName, modelPath) == false)
            {
                modelCache[levelMeshData.MeshName] = make_pair(nullptr, ResourceRef<Model>());
                continue;
            }

            modelRef = RESOURCES->GetResourceRefByAbsPath<Model>(modelPath);

            Transform* folderParent = rootTransform;
            const fs::path relativeModelPath = modelPath.lexically_relative(modelRootPath);
            if (!relativeModelPath.empty() && relativeModelPath.has_parent_path())
            {
                const string folderName = (*relativeModelPath.begin()).string();
                auto folderIt = folderCache.find(folderName);
                if (folderIt == folderCache.end())
                {
                    folderParent = CUR_SCENE->Add(folderName, rootTransform).Resolve()->GetTransform();
                    folderCache.emplace(folderName, folderParent);
                }
                else
                {
                    folderParent = folderIt->second;
                }
            }

            meshParent = CUR_SCENE->Add(levelMeshData.MeshName, folderParent).Resolve()->GetTransform();
            if (useIndividualTreeRenderer == false)
            {
                meshParent->GetGameObject()->AddComponent<ModelRenderer>().Resolve()->SetModel(modelRef);
            }

            modelCache[levelMeshData.MeshName] = make_pair(meshParent, modelRef);
        }
        else
        {
            if (it->second.first == nullptr)
                continue;

            meshParent = it->second.first;
            modelRef = it->second.second;
        }

        //GameObject* meshObj = CUR_SCENE->Add(levelMeshData.ActorName, meshParent).Resolve();
        //meshObj->AddComponent<ModelRenderer>().Resolve()->SetModel(modelRef);

        const TransformData& transformData = levelMeshData.Transform;

        // ModelImportPreset::Unreal applies ConvertToLeftHanded followed by
        // a +90 degree X rotation to the imported mesh. Use that same basis
        // conversion for the actor transform: Unreal (X, Y, Z) -> (X, Z, Y).
        Vec3 position(
            transformData.Position.X,
            transformData.Position.Z,
            transformData.Position.Y
        );
        position *= 0.01f;

        // Swapping Y/Z changes handedness. Quaternion vector components are
        // axial, so they require the additional sign inversion.
        Quaternion rotation(
            -transformData.Rotation.X,
            -transformData.Rotation.Z,
            -transformData.Rotation.Y,
            transformData.Rotation.W
        );
        rotation.Normalize();

        Vec3 scale(
            transformData.Scale.X,
            transformData.Scale.Z,
            transformData.Scale.Y
        );

        Matrix S = Matrix::CreateScale(scale);
        Matrix R = Matrix::CreateRotationY(XM_PI) *
            Matrix::CreateFromQuaternion(rotation);
        Matrix T = Matrix::CreateTranslation(position);

        Matrix worldMatrix = S * R * T;
        if (useIndividualTreeRenderer)
        {
            const string objectName = levelMeshData.ActorName.empty()
                ? levelMeshData.MeshName + "_" + std::to_string(i)
                : levelMeshData.ActorName;
            GameObject* treeObject = CUR_SCENE->Add(objectName, meshParent).Resolve();
            treeObject->AddComponent<ModelRenderer>().Resolve()->SetModel(modelRef);
            treeObject->GetTransform()->SetWorldMatrix(worldMatrix);
            ++individualTreeRendererCount;
        }
        else
        {
            meshParent->GetGameObject()->GetFixedComponent<ModelRenderer>()->AddInstancingData(worldMatrix);
        }
    }

    if (skippedBatchedInstanceCount > 0)
    {
        DBG->Log(Utils::Format(
            "[UnrealLevelImporter] Skipped %zu instance(s) already covered by Batch Info.",
            skippedBatchedInstanceCount));
    }

    if (individualTreeRendererCount > 0)
    {
        DBG->Log(Utils::Format(
            "[UnrealLevelImporter] Created %zu individual tree Model Renderer(s) for frustum culling.",
            individualTreeRendererCount));
    }

    rootTransform->SetLocalPosition(Vec3(-1, 8, 134));
    rootTransform->SetLocalRotation(Vec3(0, 90, 0));

    //auto& children = rootTransform->GetChildren();
    //vector<pair<int, string>> v;
    //for (auto& child : children)
    //{
    //    Transform* transform = child.Resolve();
    //    ModelRenderer* modelRenderer = transform->GetGameObject()->GetFixedComponent<ModelRenderer>();
    //    v.push_back(make_pair(modelRenderer->GetInstancingCount(), transform->GetGameObject()->GetName()));;
    //}
    //std::sort(v.begin(), v.end());

    return true;
}

bool UnrealLevelImporter::TryFindModelPath(const fs::path& rootPath, const string& name, fs::path& findPath)
{
    error_code error;
    if (!fs::is_directory(rootPath, error))
        return {};

    bool fbxFound = false;
    const fs::directory_options options = fs::directory_options::skip_permission_denied;
    fs::recursive_directory_iterator iterator(rootPath, options, error);
    const fs::recursive_directory_iterator end;

    while (!error && iterator != end)
    {
        const fs::directory_entry& entry = *iterator;
        if (entry.is_regular_file(error) && !error && entry.path().stem() == fs::path(name))
        {
            const string extension = ToLower(entry.path().extension().string());
            if (extension == ".model")
            {
                findPath = entry.path();
                return true;
            }

            if (extension == ".fbx")
            {
                if (fbxFound == true)
                {
                    ASSERT(false, "Multiple FBX files found for the same name: " + name);
                    return false;
                }
                findPath = entry.path();
                fbxFound = true;
            }
        }

        error.clear();
        iterator.increment(error);
    }

    return fbxFound;
}
