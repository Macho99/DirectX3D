#pragma once
#include "MetaFile.h"
#include "AnimationImportSetting.h"

class AnimationMeta : public MetaFile
{
    using Super = MetaFile;
public:
    AnimationMeta(): Super(ResourceType::Animation) {}
    ~AnimationMeta() {}

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
    virtual bool OnGUI() override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(clipImportSetting));
    }

private:
    AnimationClipImportSetting clipImportSetting;
};