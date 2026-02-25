#include "pch.h"
#include "ModelMeta.h"
#include "Converter.h"
#include <regex>

ModelMeta::ModelMeta() : Super(ResourceType::Model)
{
}

ModelMeta::~ModelMeta()
{
}

void ModelMeta::Import()
{
    Super::Import();

    wstring artifactFoloder = GetArtifactPath();
    Converter converter;
    wstring absPath = GetAbsPath();
    converter.ReadAssetFile(absPath);

    fs::create_directories(artifactFoloder);
    vector<SubAssetInfo> exported;
    converter.TryExportAll(absPath, GetArtifactPath(), OUT exported);

    for (int newIdx = 0; newIdx < exported.size(); newIdx++)
    {
        for (int prevIdx = 0; prevIdx < _subAssets.size(); prevIdx++)
        {
            if (exported[newIdx].fileName == _subAssets[prevIdx].fileName
                && exported[newIdx].resourceType == _subAssets[prevIdx].resourceType)
            {
                exported[newIdx].assetId = _subAssets[prevIdx].assetId;
                break;
            }
        }

        if (exported[newIdx].assetId.IsValid() == false)
            exported[newIdx].assetId = AssetId::CreateAssetId();
    }

    _subAssets = exported;

    for (SubAssetInfo& info : _subAssets)
    {
        if (info.resourceType == ResourceType::Material)
        {
            wstring matPath = GetArtifactPath() + L"\\" + info.fileName;
            if (fs::exists(matPath))
            {
                string matContent = Utils::ReadFile(matPath);

                string remappedMat = RemapTextureNamesToAssetId(matContent);
                Utils::WriteToFile(matPath, remappedMat);
            }
        }
    }
}

string ModelMeta::RemapTextureNamesToAssetId(string xml)
{
    std::regex re(
        R"((<(?:DiffuseFile|SpecularFile|NormalFile)>\s*)([^<]+?)(\s*</(?:DiffuseFile|SpecularFile|NormalFile)>))"
    );

    string result;
    result.reserve(xml.size());

    std::sregex_iterator it(xml.begin(), xml.end(), re);
    std::sregex_iterator end;

    size_t last = 0;

    for (; it != end; ++it)
    {
        auto& m = *it;

        size_t pos = m.position();
        size_t len = m.length();

        result.append(xml, last, pos - last);

        const string open = m[1];
        const string file = m[2];
        const string close = m[3];

        string replaced = file;

        for (SubAssetInfo& subAsset : _subAssets)
        {
            if (subAsset.resourceType == ResourceType::Texture
                && subAsset.fileName == Utils::ToWString(file))
            {
                replaced = subAsset.assetId.ToString();
                break;
            }
        }


        result += open + replaced + close;

        last = pos + len;
    }

    result.append(xml, last, std::string::npos);
    return result;
}
