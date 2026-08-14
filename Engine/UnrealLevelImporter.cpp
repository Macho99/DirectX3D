#include "pch.h"
#include "UnrealLevelImporter.h"
#include <cctype>
#include <fstream>
#include "Model.h"
#include "ModelRenderer.h"

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
    unordered_map<string, pair<Transform*, ResourceRef<Model>>> modelCache;

    for (int i = 0; i < 10000 && i < outLevel.MeshCount; i++)
    {
        const LevelMeshData& levelMeshData = outLevel.Meshes[i];
        ResourceRef<Model> modelRef;
        Transform* meshParent = nullptr;

        auto it = modelCache.find(levelMeshData.MeshName);
        if (it == modelCache.end())
        {
            fs::path modelPath;
            if (TryFindModelPath("..\\Assets\\Models\\Rural_Cabin", levelMeshData.MeshName, modelPath) == false)
            {
                modelCache[levelMeshData.MeshName] = make_pair(nullptr, ResourceRef<Model>());
                continue;
            }

            modelRef = RESOURCES->GetResourceRefByAbsPath<Model>(modelPath);
            meshParent = CUR_SCENE->Add(levelMeshData.MeshName, rootTransform).Resolve()->GetTransform();
            meshParent->GetGameObject()->AddComponent<ModelRenderer>().Resolve()->SetModel(modelRef);

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
        Matrix R = Matrix::CreateFromQuaternion(rotation);
        Matrix T = Matrix::CreateTranslation(position);

        Matrix worldMatrix = S * R * T;
        meshParent->GetGameObject()->GetFixedComponent<ModelRenderer>()->AddInstancingData(worldMatrix);
    }

    rootTransform->SetLocalPosition(Vec3(-1, 8, 134));
    rootTransform->SetLocalRotation(Vec3(0, 90, 0));
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
