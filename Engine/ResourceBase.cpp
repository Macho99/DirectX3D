#include "pch.h"
#include "ResourceBase.h"
#include "Material.h"

ResourceBase::ResourceBase(ResourceType type)
	: _type(type)
{

}

ResourceBase::~ResourceBase()
{

}

bool ResourceBase::OnGUI(bool isReadOnly)
{
	if (ImGui::Button("Save"))
	{
        DBG->Log("ResourceBase::OnGUI: Save button clicked for resource: " + _assetId.ToString());
	}

	return false;
}

CEREAL_REGISTER_TYPE(Material);
CEREAL_REGISTER_POLYMORPHIC_RELATION(ResourceBase, Material);