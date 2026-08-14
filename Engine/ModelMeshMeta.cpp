#include "pch.h"
#include "ModelMeshMeta.h"
#include "ModelMeshResource.h"

ModelMeshMeta::ModelMeshMeta()
    : Super(ResourceType::ModelMesh)
{
}

unique_ptr<ResourceBase> ModelMeshMeta::LoadResource(AssetId assetId) const
{
    if (assetId != GetAssetId())
        return nullptr;

    unique_ptr<ModelMeshResource> modelMesh = make_unique<ModelMeshResource>();
    modelMesh->ReadModel(GetAssetPath().wstring());
    modelMesh->SetName(GetAssetPath().stem().wstring());
    return modelMesh;
}

CEREAL_REGISTER_TYPE(ModelMeshMeta);
CEREAL_REGISTER_POLYMORPHIC_RELATION(MetaFile, ModelMeshMeta);
