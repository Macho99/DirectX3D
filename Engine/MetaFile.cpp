#include "pch.h"
#include "MetaFile.h"
#include "SubAssetMetaFile.h"
#include "TextureMeta.h"
#include "ModelMeta.h"
#include "FolderMeta.h"
#include "NotSupportMeta.h"
#include "fstream"
#include "MetaStore.h"

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

MetaFile::~MetaFile()
{
}

wstring MetaFile::GetResourcePath()
{
    return GetArtifactPath() + L"\\asset";
}

bool MetaFile::ImportIfDirty()
{
    bool imported = false;
    const wstring manifestPath = GetManifestPath();
    if (fs::exists(manifestPath))
    {
        try
        {
            std::ifstream is(manifestPath);
            cereal::JSONInputArchive manifestArchive(is);
            manifestArchive(_importManifest);
        }
        catch (const std::exception& e)
        {
            DBG->LogW(L"[MetaFile] ImportIfDirty: Failed to read manifest and will reimport: " + manifestPath + L", error: " + Utils::ToWString(e.what()));
            _importManifest = ImportManifest();
        }
    }

    bool isDirty = false;
    bool isManifestRefreshed = _importManifest.Refresh(_absPath, isDirty);

    if (isDirty)
    {
        Import();
        imported = true;
    }

    if (isManifestRefreshed)
    {
        std::ofstream manifestOs(manifestPath);
        cereal::JSONOutputArchive manifestArchive(manifestOs);
        manifestArchive(_importManifest);
    }
    return imported;
}

void MetaFile::ForceReImport()
{
    _importManifest = ImportManifest();
    wstring manifestPath = GetManifestPath();
    if (fs::exists(manifestPath))
        fs::remove(manifestPath);

    ImportIfDirty();
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

//CEREAL_REGISTER_TYPE(MetaFile);

//CEREAL_REGISTER_TYPE(SubAssetMetaFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, SubAssetMetaFile);

CEREAL_REGISTER_TYPE(ModelMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(SubAssetMetaFile, ModelMeta);

CEREAL_REGISTER_TYPE(TextureMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, TextureMeta);

CEREAL_REGISTER_TYPE(FolderMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, FolderMeta);

CEREAL_REGISTER_TYPE(NotSupportMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, NotSupportMeta);