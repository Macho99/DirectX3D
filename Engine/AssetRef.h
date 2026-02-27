#pragma once
#include "AssetId.h"
#include "Handle.h"

struct AssetRef
{
public:
    AssetRef() : assetId(), cached() {}
    AssetRef(AssetId assetId) : assetId(assetId), cached() {}
    AssetRef(const AssetRef& other) : assetId(other.assetId), cached(other.cached) {}
    AssetRef(AssetId assetId, Handle handle) : assetId(assetId), cached(handle) {}

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(assetId));
    }

    bool operator==(const AssetRef& rhs) const noexcept
    {
        return assetId == rhs.assetId;
    }

    bool operator!=(const AssetRef& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    bool IsValid() const { return assetId.IsValid(); }

    ResourceBase* Resolve();

    AssetId GetAssetId() const { return assetId; }

protected:
    AssetId assetId;
    mutable Handle cached;
};

//struct AssetRefEq
//{
//    bool operator()(const AssetRef& a, const AssetRef& b) const noexcept
//    {
//        return a.assetId == b.assetId;
//    }
//};
//
//struct AssetRefHash
//{
//    std::size_t operator()(const AssetRef& r) const noexcept
//    {
//        return AssetIdHash{}(r.assetId);
//    }
//};