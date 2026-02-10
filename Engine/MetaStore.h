#pragma once
#include "MetaFile.h"

class MetaStore
{
public:
    static fs::path MetaPathForSource(const fs::path& sourceAbs); 
    static fs::path SourcePathForMeta(const fs::path& metaAbs);

    // meta 로드 (없거나 파싱 실패면 nullopt)
    static std::optional<MetaFile> TryLoad(const fs::path& metaAbs);

    // meta 저장 (원자적 저장: tmp -> replace)
    static bool SaveAtomic(const fs::path& metaAbs, const MetaFile& meta);

    // meta가 없으면 생성, 있으면 로드해서 반환
    static MetaFile LoadOrCreate(const fs::path& sourceAbs);
    static bool IsMetaFile(const fs::path& path);
};
