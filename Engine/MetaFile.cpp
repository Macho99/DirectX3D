#include "pch.h"
#include "MetaFile.h"
#include "SubAssetMetaFile.h"
#include "TextureMeta.h"
#include "ModelMeta.h"

MetaFile::MetaFile()
    :_resourceType(ResourceType::None)
{
}

MetaFile::MetaFile(ResourceType resourceType)
    :_resourceType(resourceType)
{
}

wstring MetaFile::GetResourcePath()
{
    return GetArtifactPath() + L"\\asset";
}

wstring MetaFile::GetArtifactPath()
{
    if (!_assetId.IsValid())
        assert(false && "MetaFile::GetArtifactPath: invalid guid");

    return L"..\\Library\\" + _assetId.ToWString();
}

CEREAL_REGISTER_TYPE(MetaFile);

//CEREAL_REGISTER_TYPE(SubAssetMetaFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, SubAssetMetaFile);

CEREAL_REGISTER_TYPE(ModelMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(SubAssetMetaFile, ModelMeta);

CEREAL_REGISTER_TYPE(TextureMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, TextureMeta);