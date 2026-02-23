#include "pch.h"
#include "TextureMeta.h"

unique_ptr<Texture> TextureMeta::LoadIconTexture() const
{
    unique_ptr<Texture> texture = make_unique<Texture>();
    texture->Load(GetAbsPath());
    return texture;
}
