#include "pch.h"
#include "SceneMeta.h"

unique_ptr<ResourceBase> SceneMeta::LoadResource(AssetId assetId) const
{
    ASSERT(false, "씬은 리소스로 로드할 수 없음");
    return nullptr;
}
