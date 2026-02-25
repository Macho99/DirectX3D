#include "pch.h"
#include "ShaderMeta.h"

ShaderMeta::ShaderMeta()
    : Super(ResourceType::Shader)
{
}

ShaderMeta::~ShaderMeta()
{
}

unique_ptr<ResourceBase> ShaderMeta::LoadResource() const
{
    //unique_ptr<Shader> shader = make_unique<Shader>(GetAbsPath());
    return nullptr;
}
