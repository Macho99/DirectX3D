#include "pch.h"
#include "Guid.h"
#include "Utils.h"


uint64 Guid::s_currentInstanceId = 0;

Guid Guid::CreateNewObjectGuid()
{
    Guid newGuid;
    newGuid.instanceId = s_currentInstanceId;
    newGuid.localId = Utils::GetRandomUInt64();
    return newGuid;
}

Guid Guid::CreateNewAssetGuid()
{
    Guid newGuid;
    newGuid.instanceId = Utils::GetRandomUInt64();
    newGuid.localId = Utils::GetRandomUInt64();
    return newGuid;
}

void Guid::SetCurrentInstanceId(uint64 id)
{ 
    s_currentInstanceId = id;
}
