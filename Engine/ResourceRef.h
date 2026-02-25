#pragma once
#include "AssetRef.h"

template<typename T>
using EnableIfResourceBase = std::enable_if_t<std::is_base_of<ResourceBase, T>::value, int>;

template<class T, EnableIfResourceBase<T> = 0>
struct ResourceRef : public AssetRef
{
    using Super = AssetRef;
public:
    ResourceRef() = default;
    ResourceRef(const AssetId& assetId) : AssetRef(assetId) {}

    static ResourceRef CreateByPath(const fs::path& absPath)
    {
        AssetId assetId;
        if (!RESOURCES->TryGetAssetIdByPath(absPath, OUT assetId))
            assetId = AssetId();
        return ResourceRef(assetId);
    }

    T* Resolve() const
    {
        return static_cast<T*>(Super::Resolve());
    }
};