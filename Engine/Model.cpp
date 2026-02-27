#include "pch.h"
#include "Model.h"
#include "Utils.h"
#include "FileUtils.h"
#include "tinyxml2.h"
#include <filesystem>
#include "Material.h"
#include "ModelMesh.h"
#include "ModelAnimation.h"

Model::Model()
    : Super(ResourceType::Model)
{

}

Model::Model(vector<ResourceRef<Material>> materials, ResourceRef<ModelMeshResource> mesh, vector<ResourceRef<ModelAnimation>> animations)
    :Super(ResourceType::Model), _materials(materials), _mesh(mesh), _animations(animations)
{
	BindCache();
}

Model::~Model()
{

}

void Model::BindCache()
{
    ModelMeshResource* mesh = _mesh.Resolve();
    if (mesh == nullptr)
		return;
    mesh->BindCacheInfo(_materials);
}

ResourceRef<Material> Model::GetMaterialByName(const wstring& name)
{
	for (auto& materialRef : _materials)
	{
        Material* material = materialRef.Resolve();

		if (material->GetName() == name)
			return materialRef;
	}

	return MaterialRef();
}

ModelAnimation* Model::GetAnimationByName(wstring name)
{
	for (ResourceRef<ModelAnimation>& animationRef : _animations)
	{
        ModelAnimation* anim = animationRef.Resolve();
		if (anim->GetName() == name)
			return anim;
	}

	return nullptr;
}