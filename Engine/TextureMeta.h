#pragma once
#include "MetaFile.h"
class TextureMeta : public MetaFile
{
public:
    TextureMeta() : MetaFile(ResourceType::Texture) {}
    ~TextureMeta() {}

    wstring GetResourcePath() override
    {
        return L"Textures\\" + GetAssetId().ToWString() + L".tex";
    }
};

