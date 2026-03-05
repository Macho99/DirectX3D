#pragma once
#include "MetaFile.h"

struct SubAssetInfo
{
    SubAssetInfo()
        :assetId(), fileName(L""), resourceType(ResourceType::None)
    {

    }

    AssetId assetId;
    wstring fileName;
    ResourceType resourceType;
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(assetId));
        string utf8;
        if constexpr (Archive::is_saving::value)
            utf8 = Utils::ToString(fileName);

        ar(cereal::make_nvp("fileName", utf8));

        if constexpr (Archive::is_loading::value)
            fileName = Utils::ToWString(utf8);
        ar(CEREAL_NVP(resourceType));
    }
};

class SubAssetMetaFile : public MetaFile
{
    using Super = MetaFile;
    using Super::LoadResource;
public:
    SubAssetMetaFile(ResourceType resourceType)
        :Super(resourceType)
    {
    }
    virtual void OnLoad(unordered_map<AssetId, MetaFile*, AssetIdHash>& subAssetContainer) override;
    virtual void OnDestroy(unordered_map<AssetId, MetaFile*, AssetIdHash>& subAssetContainer) override;

    wstring GetResourcePath() const override
    {
        assert(false && "SubAssetMetaFile has no single resource path");
        return L"";
    }

    wstring GetSubResourcePath(int index) const;

    virtual void DrawContentBrowserItem(fs::path& _currentFolder, float _thumbSize, int& curCol, int columns) const;
    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;
    AssetId GetSubAssetIdByIndex(int subAssetIdx) const;
    int GetSubAssetIndexById(const AssetId& assetId) const;
    virtual string GetName(const AssetId& assetId) override;

    bool TryGetSubAssetByType(ResourceType resourceType, OUT AssetId& assetId) const;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_subAssets));
    }

protected:
    vector<SubAssetInfo> _subAssets;
};