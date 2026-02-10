#pragma once
#include "MetaFile.h"
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

protected:
    vector<ResourceType> _subAssets;
};