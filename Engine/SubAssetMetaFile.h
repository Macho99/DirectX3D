#pragma once
#include "MetaFile.h"

struct SubAssetInfo
{
    AssetId assetId;
    ResourceType resourceType;
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(assetId));
        ar(CEREAL_NVP(resourceType));
    }
};

class SubAssetMetaFile : public MetaFile
{
    using Super = MetaFile;
public:
    SubAssetMetaFile(ResourceType resourceType)
        :Super(resourceType)
    {
    }

    wstring GetResourcePath() override
    {
        assert(false && "SubAssetMetaFile has no single resource path");
        return L"";
    }

    wstring GetSubResourcePath(int index)
    {
        if (index < 0 || index >= (int)_subAssets.size())
        {
            DBG->LogErrorW(L"[SubAssetMetaFile] GetResourcePath: index out of range: " + std::to_wstring(index));
            return L"";
        }
        return GetArtifactPath() + L"\\subAsset_" + to_wstring(index);
    }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_subAssets));
    }

protected:
    vector<SubAssetInfo> _subAssets;
};