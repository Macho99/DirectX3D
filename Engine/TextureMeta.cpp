#include "pch.h"
#include "TextureMeta.h"

unique_ptr<ResourceBase> TextureMeta::LoadResource() const
{
    unique_ptr<Texture> texture = make_unique<Texture>();
    texture->Load(_absPath);
    return texture;
}
