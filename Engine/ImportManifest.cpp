#include "pch.h"
#include "ImportManifest.h"

bool ImportManifest::Refresh(fs::path filePath, OUT bool& isDirty)
{
    isDirty = false;
    if (!fs::exists(filePath))
    {
        assert(false && "File does not exist");
    }

    uint64 curFileSize = fs::file_size(filePath);
    uint64 curFileTime = fs::last_write_time(filePath).time_since_epoch().count();

    if (curFileSize != size || curFileTime != timestamp)
    {
        size = curFileSize;
        timestamp = curFileTime;

        string newHash = Utils::CalcFileHash(filePath);
        if (newHash != hash)
        {
            hash = newHash;
            isDirty = true;
        }
        return true;
    }

    return false;
}
