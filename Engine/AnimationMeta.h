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
    virtual int GetImportVersion() const override;
    virtual int GetSerializeVersion() const override { return 3; }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        clipImportSetting.version = _version;
        ar(CEREAL_NVP(clipImportSetting));

        if(_version >= 2)
            ar(CEREAL_NVP(animationEvents));
    }

protected:
    AnimationClipImportSetting clipImportSetting;
    vector<AnimationEvent> animationEvents;
};
