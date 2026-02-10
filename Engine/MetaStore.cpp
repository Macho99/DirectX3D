#include "pch.h"
#include "MetaStore.h"
#include "fstream"

fs::path MetaStore::MetaPathForSource(const fs::path& sourceAbs)
{
    // Unity 스타일: "<file>.ext.meta"
    return fs::path(sourceAbs.wstring() + L".meta");
}

fs::path MetaStore::SourcePathForMeta(const fs::path& metaAbs)
{
    // "<file>.ext.meta" 에서 ".meta"만 제거 => "<file>.ext"
    fs::path src = metaAbs;
    if (src.extension() == L".meta")
        src.replace_extension(); // .meta 제거
    return src;
}

static bool ParseLineU64(const std::string& line, const char* key, uint64_t& out)
{
    // key=VALUE 형태
    const std::string prefix = std::string(key) + "=";
    if (line.rfind(prefix, 0) != 0) return false;
    try
    {
        out = std::stoull(line.substr(prefix.size()));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::optional<MetaFile> MetaStore::TryLoad(const fs::path& metaAbs)
{
    std::ifstream in(metaAbs);
    if (!in.is_open()) return std::nullopt;

    MetaFile m{};
    std::string line;
    bool gotHi = false, gotLo = false;
    uint64 instanceId = 0;
    uint64 localId = 0;

    while (std::getline(in, line))
    {
        uint64_t v = 0;
        if (!gotHi && ParseLineU64(line, "guid_hi", v))
        {
            instanceId = v;
            gotHi = true;
            continue;
        }
        if (!gotLo && ParseLineU64(line, "guid_lo", v))
        {
            localId = v;
            gotLo = true;
            continue;
        }
    }
    m.guid = Guid(instanceId, localId);
    if (!gotHi || !gotLo) return std::nullopt;
    if (!m.guid.IsValid()) return std::nullopt;
    return m;
}

bool MetaStore::SaveAtomic(const fs::path& metaAbs, const MetaFile& meta)
{
    fs::path tmp = fs::path(metaAbs.wstring() + L".tmp");

    {
        std::ofstream out(tmp);
        if (!out.is_open()) return false;

        out << "guid_hi=" << meta.guid.GetInstanceId() << "\n";
        out << "guid_lo=" << meta.guid.GetLocalId() << "\n";
        out.flush();
        if (!out) return false;
    }

    // tmp -> meta replace
    BOOL ok = ::MoveFileExW(tmp.c_str(), metaAbs.c_str(), MOVEFILE_REPLACE_EXISTING);
    return ok == TRUE;
}

MetaFile MetaStore::LoadOrCreate(const fs::path& sourceAbs)
{
    fs::path metaAbs = MetaPathForSource(sourceAbs);

    if (auto loaded = TryLoad(metaAbs))
        return *loaded;

    MetaFile m{};
    m.guid = Guid::CreateNewAssetGuid();
    SaveAtomic(metaAbs, m);
    return m;
}

bool MetaStore::IsMetaFile(const fs::path& path)
{
    return path.extension() == L".meta";
}
