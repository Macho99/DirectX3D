#include "pch.h"
#include "ResourceBase.h"

ResourceBase::ResourceBase(ResourceType type)
	: _type(type)
{

}

ResourceBase::~ResourceBase()
{

}

void ResourceBase::OnGUI(bool isReadOnly)
{
	ImGui::Text("No inspector");
}
