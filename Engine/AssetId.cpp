#include "pch.h"
#include "AssetId.h"

AssetId AssetId::CreateAssetId()
{
    return AssetId(Utils::GetRandomUInt64(), Utils::GetRandomUInt64());
}

bool AssetId::TryParse(const string& str, OUT AssetId& out)
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
    out = AssetId(high, low);
    return true;
}
