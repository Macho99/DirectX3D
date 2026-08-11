#pragma once
#include "MetaFile.h"
class TextureMeta : public MetaFile
{
    using Super = MetaFile;
public:
    TextureMeta() : MetaFile(ResourceType::Texture) {}
    ~TextureMeta() {}

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
    virtual fs::path GetImportedAssetPath(const AssetId& assetId) const override;
    virtual Texture* GetIconTexture() const override;
    virtual bool OnGUI() override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        if (_version >= 3)
            ar(CEREAL_NVP(_keepCpuPixels));
    }

protected:
    virtual void Import() override;
    virtual int GetVersion() const override { return 8; }

private:
    fs::path GetThumbnailPath() const;
    bool _keepCpuPixels = false;
};

