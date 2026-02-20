#include "pch.h"
#include "MetaFile.h"
#include "SubAssetMetaFile.h"
#include "TextureMeta.h"
#include "ModelMeta.h"
#include "fstream"

MetaFile::MetaFile()
    :_resourceType(ResourceType::None)
    , _importManifest()
    , _assetId()
{
}

MetaFile::MetaFile(ResourceType resourceType)
    :_resourceType(resourceType)
    , _importManifest()
    , _assetId()
{
}

wstring MetaFile::GetResourcePath()
{
    return GetArtifactPath() + L"\\asset";
}

void MetaFile::ImportIfDirty()
{
    wstring manifestPath = GetManifestPath();
    if (fs::exists(manifestPath))
    {
        std::ifstream is(manifestPath);
        cereal::JSONInputArchive manifestArchive(is);
        manifestArchive(_importManifest);
    }

    bool isDirty = false;
    bool isRefreshed = _importManifest.Refresh(_absPath, isDirty);

    if (isDirty)
    {
        Import();
    }

    if (isRefreshed)
    {
        std::ofstream manifestOs(GetManifestPath());
        cereal::JSONOutputArchive manifestArchive(manifestOs);
        manifestArchive(_importManifest);
    }
}

void MetaFile::ForceReImport()
{

}

void MetaFile::Import()
{
    wstring artifactFolder = GetArtifactPath();
    if (fs::exists(artifactFolder))
        fs::remove_all(artifactFolder);

    fs::create_directories(artifactFolder);
}

wstring MetaFile::GetArtifactPath()
{
    if (!_assetId.IsValid())
        assert(false && "MetaFile::GetArtifactPath: invalid guid");

    return L"..\\Artifact\\" + _assetId.ToWString();
}

CEREAL_REGISTER_TYPE(MetaFile);

//CEREAL_REGISTER_TYPE(SubAssetMetaFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, SubAssetMetaFile);

CEREAL_REGISTER_TYPE(ModelMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(SubAssetMetaFile, ModelMeta);

CEREAL_REGISTER_TYPE(TextureMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, TextureMeta);