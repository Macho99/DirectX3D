#include "pch.h"
#include "AssetSlot.h"
#include "MetaFile.h"

AssetSlot::AssetSlot()
{
}

AssetSlot::~AssetSlot()
{
}

void AssetSlot::OnDestroy()
{
    for (int i = _slots.size() - 1; i >= 0; i--)
    {
        _slots[i].ptr.reset();
    }
    _freeIndices.clear();
    _assetIdToHandle.clear();
}

Handle AssetSlot::FindHandle(const AssetId& assetId)
{
    auto it = _assetIdToHandle.find(assetId);
    if (it == _assetIdToHandle.end())
    {
        return Register(assetId);
    }
    return it->second;
}

ResourceBase* AssetSlot::Resolve(const Handle& h)
{
    if (!h.IsValid()) return nullptr;
    if (h.index >= _slots.size()) return nullptr;

    Slot& s = _slots[h.index];
    if (s.gen != h.gen) return nullptr;
    if (!s.ptr) return nullptr;
    return s.ptr.get();
}

void AssetSlot::Remove(const AssetId& assetId)
{
    auto it = _assetIdToHandle.find(assetId);
    if (it == _assetIdToHandle.end())
        return;
    Handle h = it->second;
    Slot& slot = _slots[h.index];
    // 슬롯 초기화
    slot.ptr.reset();
    slot.gen++;  // 세대 증가
    _freeIndices.push_back(h.index);
    _assetIdToHandle.erase(it);
}

Handle AssetSlot::AllocateSlot()
{
    if (!_freeIndices.empty())
    {
        uint32 idx = _freeIndices.back();
        _freeIndices.pop_back();
        return Handle{ idx, _slots[idx].gen };
    }

    uint32 idx = static_cast<uint32>(_slots.size());
    _slots.push_back(Slot{});
    return Handle{ idx, _slots[idx].gen };
}

Handle AssetSlot::Register(AssetId assetId)
{
    Handle handle = AllocateSlot();
    Slot& slot = _slots[handle.index];

    MetaFile* meta = nullptr;
    unique_ptr<ResourceBase> obj = nullptr;
    if (RESOURCES->TryGetMetaByAssetId(assetId, OUT meta))
    {
        obj = meta->LoadResource(assetId);

        if (obj != nullptr)
            obj->SetId(assetId);
    }

    slot.ptr = std::move(obj);

    _assetIdToHandle[assetId] = handle;
    return handle;
}

AssetRef AssetSlot::Register(unique_ptr<ResourceBase> resource)
{
    Handle handle = AllocateSlot();
    Slot& slot = _slots[handle.index];
    AssetId assetId = AssetId::CreateAssetId();
    resource->SetId(assetId);

    slot.ptr = std::move(resource);

    _assetIdToHandle[assetId] = handle;
    return AssetRef(assetId, handle);
}
