#pragma once

enum class ModelImportPreset : uint8
{
    Default,
    Unreal,
    Custom,
};

inline const char* const ModelImportPresetNames[] =
{
    "Default",
    "Unreal",
    "Custom",
};

struct ModelImportSettings
{
    ModelImportPreset preset = ModelImportPreset::Default;
    float scale = 1.0f;
    Vec3 rotation = Vec3::Zero;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(preset),
            CEREAL_NVP(scale),
            CEREAL_NVP(rotation)
        );
    }
};
