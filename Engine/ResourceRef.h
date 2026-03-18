#pragma once
#include "AssetRef.h"
#include "ResourceBase.h"

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
    friend class ModelSourceMeta;
};