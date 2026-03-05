#include "pch.h"
#include "ImportManifest.h"

bool ImportManifest::Refresh(fs::path filePath, int curVersion, OUT bool& isDirty)
{
    isDirty = false;
    if (!fs::exists(filePath))
    {
        assert(false && "File does not exist");
    }

    uint64 curFileSize = fs::file_size(filePath);
    uint64 curFileTime = fs::last_write_time(filePath).time_since_epoch().count();

    if (curVersion != version || curFileSize != size || curFileTime != timestamp)
    {
        if (curVersion != version)
            isDirty = true;

        version = curVersion;
        size = curFileSize;
        timestamp = curFileTime;

        if (fs::is_directory(filePath))
        {
            hash = ""; // 디렉토리는 해시 계산 안함
            isDirty = true;
        }
        else
        {
            string newHash = Utils::CalcFileHash(filePath);

            if (newHash != hash)
            {
                hash = newHash;
                isDirty = true;
            }
        }
        return true;
    }

    return false;
}
