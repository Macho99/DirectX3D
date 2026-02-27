#pragma once
#include "AssetRef.h"
#include "ResourceBase.h"

template<typename T>
struct ResourceTypeTrait
{
    static constexpr ResourceType value = ResourceType::None;
};

class Texture;
class Shader;
class Mesh;
class Material;
class ModelAnimation;
class ModelMesh;

template<>
struct ResourceTypeTrait<Texture>
{
    static constexpr ResourceType value = ResourceType::Texture;
};

template<>
struct ResourceTypeTrait<Shader>
{
    static constexpr ResourceType value = ResourceType::Shader;
};

template<>
struct ResourceTypeTrait<Mesh>
{
    static constexpr ResourceType value = ResourceType::Mesh;
};

template<>
struct ResourceTypeTrait<Material>
{
    static constexpr ResourceType value = ResourceType::Material;
};

template<>
struct ResourceTypeTrait<ModelAnimation>
{
    static constexpr ResourceType value = ResourceType::Animation;
};

template<>
struct ResourceTypeTrait<ModelMesh>
{
    static constexpr ResourceType value = ResourceType::ModelMesh;
};

template<class T>
struct ResourceRef : public AssetRef
{
    using Super = AssetRef;
public:
    ResourceRef() = default;
    ResourceRef(const ResourceRef& other) : AssetRef(other.assetId, other.cached) {}

protected:
    ResourceRef(const AssetRef& assetRef) : AssetRef(assetRef) {}
    ResourceRef(const AssetId& assetId) : AssetRef(assetId) {}

public:
    T* Resolve()
    {
        return static_cast<T*>(Super::Resolve());
    }

    friend class ResourceManager;
    friend class Converter;
};