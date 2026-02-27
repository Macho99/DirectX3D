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

    wstring GetResourcePath() const override
    {
        assert(false && "SubAssetMetaFile has no single resource path");
        return L"";
    }

    wstring GetSubResourcePath(int index) const
    {
        if (index < 0 || index >= (int)_subAssets.size())
        {
            DBG->LogErrorW(L"[SubAssetMetaFile] GetResourcePath: index out of range: " + std::to_wstring(index));
            return L"";
        }
        return GetArtifactPath() + L"\\" + _subAssets[index].fileName;
    }

    virtual void DrawContentBrowserItem(fs::path& _selectedPath, fs::path& _currentFolder, float _thumbSize, int& curCol, int columns) const;
    virtual unique_ptr<ResourceBase> LoadResource(AssetId assetId) const override;

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_subAssets));
    }

protected:
    vector<SubAssetInfo> _subAssets;
};