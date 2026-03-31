#include "pch.h"
#include "ResourceBase.h"
#include "Material.h"
#include "Model.h"
#include "TerrainData.h"
#include "Mesh.h"

ResourceBase::ResourceBase()
    : _type(ResourceType::None)
{
}

ResourceBase::ResourceBase(ResourceType type)
	: _type(type)
{

}

ResourceBase::~ResourceBase()
{

}

bool ResourceBase::OnGUI(bool isReadOnly)
{
	return false;
}

void ResourceBase::OnMenu(bool isReadOnly)
{
}

CEREAL_REGISTER_TYPE(Material);
CEREAL_REGISTER_POLYMORPHIC_RELATION(ResourceBase, Material);

CEREAL_REGISTER_TYPE(Model);
CEREAL_REGISTER_POLYMORPHIC_RELATION(ResourceBase, Model);

CEREAL_REGISTER_TYPE(TerrainData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(ResourceBase, TerrainData);

CEREAL_REGISTER_TYPE(Mesh);
CEREAL_REGISTER_POLYMORPHIC_RELATION(ResourceBase, Mesh);

CEREAL_REGISTER_TYPE(Scene);
CEREAL_REGISTER_POLYMORPHIC_RELATION(ResourceBase, Scene);
