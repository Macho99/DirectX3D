#pragma once

struct Vec3Data
{
    float X;
    float Y;
    float Z;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("X", X),
            cereal::make_nvp("Y", Y),
            cereal::make_nvp("Z", Z)
        );
    }
};

struct QuatData
{
    float X;
    float Y;
    float Z;
    float W;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("X", X),
            cereal::make_nvp("Y", Y),
            cereal::make_nvp("Z", Z),
            cereal::make_nvp("W", W)
        );
    }
};

struct TransformData
{
    Vec3Data Position;
    QuatData Rotation;
    Vec3Data Scale;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("Position", Position),
            cereal::make_nvp("Rotation", Rotation),
            cereal::make_nvp("Scale", Scale)
        );
    }
};

struct LevelMeshData
{
    string ActorName;
    string ComponentName;
    string MeshName;
    string MeshPath;
    string Type;
    int InstanceIndex;

    TransformData Transform;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("ActorName", ActorName),
            cereal::make_nvp("ComponentName", ComponentName),
            cereal::make_nvp("MeshName", MeshName),
            cereal::make_nvp("MeshPath", MeshPath),
            cereal::make_nvp("Type", Type),
            cereal::make_nvp("InstanceIndex", InstanceIndex),
            cereal::make_nvp("Transform", Transform)
        );
    }
};

struct LevelData
{
    std::string LevelName;
    int MeshCount;
    std::vector<LevelMeshData> Meshes;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("LevelName", LevelName),
            cereal::make_nvp("MeshCount", MeshCount),
            cereal::make_nvp("Meshes", Meshes)
        );
    }
};

class UnrealLevelImporter
{
public:
    static bool LoadLevel(const string& path);
    static bool TryFindModelPath(const fs::path& rootPath, const string& name, fs::path& findPath);
};

