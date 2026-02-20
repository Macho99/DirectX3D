#pragma once
#include "MetaFile.h"

class MetaStore
{
    using Creator = function<unique_ptr<MetaFile>()>;
public:
    static fs::path MetaPathForSource(const fs::path& sourceAbs); 
    static fs::path SourcePathForMeta(const fs::path& metaAbs);

    static unique_ptr<MetaFile> TryLoad(const fs::path& metaAbs);
    static void Save(const fs::path& metaAbs, const MetaFile& meta);
    static unique_ptr<MetaFile> Create(const fs::path& sourceAbs);
    //static unique_ptr<MetaFile> LoadOrCreate(const fs::path& sourceAbs);
    static bool IsMetaFile(const fs::path& path);

    //static wstring GetResourceExtension(ResourceType resourceType);

private:
    static const unordered_map<string, Creator>& InitAndGetCreators();
    static unordered_map<string, Creator> _creators;
};
