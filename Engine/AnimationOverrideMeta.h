#pragma once
#include "AnimationMeta.h"
#include "ResourceRef.h"

class Model;
class ModelAnimation;
class ModelMeshResource;

struct AnimationOverrideMetaLayer
{
    string rootBoneName;
    ResourceRef<ModelAnimation> animation;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(rootBoneName));
        ar(CEREAL_NVP(animation));
    }
};

class AnimationOverrideMeta : public AnimationMeta
{
    using Super = AnimationMeta;
public:
    static string GetExtension() { return ".clipOverride"; }
    AnimationOverrideMeta() : Super() {}
    ~AnimationOverrideMeta() {}

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
    virtual bool OnGUI() override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_model));
        ar(CEREAL_NVP(_baseAnimation));
        ar(CEREAL_NVP(_overrideLayers));
    }

private:
    bool BuildAnimation(
        ModelAnimation& output,
        const ModelMeshResource& modelMesh,
        const ModelAnimation& baseAnimation,
        wstring& error) const;

    ResourceRef<Model> _model;
    ResourceRef<ModelAnimation> _baseAnimation;
    vector<AnimationOverrideMetaLayer> _overrideLayers;
};

