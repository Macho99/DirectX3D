#pragma once
#include "AssetRef.h"
#include "Handle.h"

class ResourceBase;

class AssetSlot
{
    struct Slot
    {
        unique_ptr<ResourceBase> ptr;
        uint32 gen = 1;
    };

public:
    AssetSlot();
    ~AssetSlot();

    void OnDestroy();

    // Guid로 핸들 얻기
    Handle FindHandle(const AssetId& assetId);

    // Handle -> 포인터 (세대 검사)
    ResourceBase* Resolve(const Handle& h);

    const ResourceBase* Resolve(const Handle& h) const
    {
        return const_cast<AssetSlot*>(this)->Resolve(h);
    }

    //// AssetId -> 포인터 (내부에서 Handle 찾고 Resolve)
    //ResourceBase* Resolve(const AssetId& assetId)
    //{
    //    return Resolve(FindHandle(assetId));
    //}

    void Remove(const AssetRef& assetRef) { Remove(assetRef.GetAssetId()); }
    void Remove(const AssetId& assetId);
    AssetRef Register(unique_ptr<ResourceBase> resource);

private:
    Handle AllocateSlot();
    Handle Register(AssetId assetId);

private:
    vector<Slot> _slots;
    vector<uint32> _freeIndices;
    unordered_map<AssetId, Handle, AssetIdHash> _assetIdToHandle;
};