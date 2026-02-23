#pragma once
#include "MetaFile.h"
class TextureMeta : public MetaFile
{
public:
    TextureMeta() : MetaFile(ResourceType::Texture) {}
    ~TextureMeta() {}

protected:
    virtual string GetIconKey() const override { return _assetId.ToString(); }
    virtual unique_ptr<Texture> LoadIconTexture() const override;
};

