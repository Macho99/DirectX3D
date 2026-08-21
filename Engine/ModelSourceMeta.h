#pragma once
#include "SubAssetMetaFile.h"
#include "ModelImportSettings.h"

class ModelSourceMeta : public SubAssetMetaFile
{
    using Super = SubAssetMetaFile;
public:
    ModelSourceMeta();
    ~ModelSourceMeta();

    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
    virtual bool OnGUI() override;
    virtual void OnMenu() override;
    virtual bool IsReadOnly() const override { return true; }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

        if(_version >= 15)
            ar(CEREAL_NVP(_importSettings));
    }

protected:
    void Import() override;

private:
    virtual int GetImportVersion() const override;
    virtual int GetSerializeVersion() const override { return 15; }

    ModelImportSettings _importSettings;
};

