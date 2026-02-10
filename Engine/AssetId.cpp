#include "pch.h"
#include "AssetId.h"

AssetId AssetId::CreateAssetId()
{
    return AssetId(Utils::GetRandomUInt64(), Utils::GetRandomUInt64());
}
