#include "pch.h"
#include "Guid.h"
#include "Utils.h"


uint64 Guid::s_currentInstanceId = 0;

Guid Guid::CreateGuid()
{
    Guid newGuid;
    newGuid.instanceId = s_currentInstanceId;
    newGuid.localId = Utils::GetRandomUInt64();
    return newGuid;
}

bool Guid::TryParse(const string& str, OUT Guid& guid)
{
    size_t sep = str.find('_');
    if (sep == string::npos)
        return false;
    string highStr = str.substr(0, sep);
    string lowStr = str.substr(sep + 1);
    uint64 high, low;
    try
    {
        high = stoull(highStr);
        low = stoull(lowStr);
    }
    catch (const std::exception&)
    {
        return false;
    }
    guid = Guid(high, low);
    return true;

}

void Guid::SetCurrentInstanceId(uint64 id)
{ 
    s_currentInstanceId = id;
}
