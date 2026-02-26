#pragma once
#include "AssetRef.h"

//template<typename T>
//using EnableIfResourceBase = std::enable_if_t<std::is_base_of<ResourceBase, T>::value, int>;

template<class T>
struct ResourceRef : public AssetRef
{
    using Super = AssetRef;
public:
    ResourceRef() = default;
    ResourceRef(const AssetId& assetId) : AssetRef(assetId) {}
    ResourceRef(const AssetRef& other) : AssetRef(other) {}

    T* Resolve()
    {
        return static_cast<T*>(Super::Resolve());
    }
};