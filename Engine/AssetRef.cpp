#include "pch.h"
#include "AssetRef.h"

ResourceBase* AssetRef::Resolve()
{
    if (!assetId.IsValid())
        return nullptr;

    AssetSlot& assetSlot = RESOURCES->GetAssetSlot();
    if (cached.IsValid() == false)
        cached = assetSlot.FindHandle(assetId);

    return assetSlot.Resolve(cached);
}
