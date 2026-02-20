#include "pch.h"
#include "MetaStore.h"
#include "fstream"
#include "cereal/types/polymorphic.hpp"
#include "cereal/archives/json.hpp"
#include "ModelMeta.h"
#include "TextureMeta.h"

unordered_map<string, MetaStore::Creator> MetaStore::_creators;

fs::path MetaStore::MetaPathForSource(const fs::path& sourceAbs)
{
    // Unity 스타일: "<file>.ext.meta"
    return fs::path(sourceAbs.wstring() + L".meta");
}

fs::path MetaStore::SourcePathForMeta(const fs::path& metaAbs)
{
    // "<file>.ext.meta" 에서 ".meta"만 제거 => "<file>.ext"
    fs::path src = metaAbs;
    if (src.extension() == L".meta")
        src.replace_extension(); // .meta 제거
    return src;
}

unique_ptr<MetaFile> MetaStore::TryLoad(const fs::path& metaAbs)
{
    std::ifstream is(metaAbs);
    if (!is.is_open())
        return nullptr;

    cereal::JSONInputArchive archive(is);
    unique_ptr<MetaFile> meta;
    archive(meta);

    meta->_absPath = SourcePathForMeta(metaAbs);
    meta->ImportIfDirty();

    return meta;
}

void MetaStore::Save(const fs::path& metaAbs, const MetaFile& meta)
{
    std::ofstream os(metaAbs);
    cereal::JSONOutputArchive archive(os);
    archive(meta);
}

unique_ptr<MetaFile> MetaStore::Create(const fs::path& sourceAbs)
{
    string ext = sourceAbs.extension().string();
    const auto& creators = InitAndGetCreators();
    auto it = creators.find(ext);
    unique_ptr<MetaFile> meta;
    if (it != creators.end())
    {
        meta = (*it).second();
    }
    else
    {
        meta = make_unique<MetaFile>();
    }

    meta->_assetId = AssetId::CreateAssetId();
    meta->_absPath = sourceAbs;

    meta->ImportIfDirty();

    std::ofstream os(MetaPathForSource(sourceAbs));
    cereal::JSONOutputArchive archive(os);
    archive(meta);
    return meta;
}

//unique_ptr<MetaFile> MetaStore::LoadOrCreate(const fs::path& sourceAbs)
//{
//    fs::path metaAbs = MetaPathForSource(sourceAbs);
//    unique_ptr<MetaFile> meta = TryLoad(metaAbs);
//    if (meta != nullptr && meta->GetAssetId().IsValid())
//        return meta;
//
//    meta = Create(sourceAbs);
//    return meta;
//}

bool MetaStore::IsMetaFile(const fs::path& path)
{
    return path.extension() == L".meta";
}

wstring MetaStore::GetResourceExtension(ResourceType resourceType)
{
    switch (resourceType)
    {
        case ResourceType::Mesh:        return L".mesh";
        case ResourceType::Material:    return L".mat";
        case ResourceType::Shader:      return L".fx";
        case ResourceType::Animation:   return L".clip";
    }

    assert(resourceType == ResourceType::Texture, "Get Texture Extension Not Support");
    assert(false, "GetResourceExtension: Unknown resource type");
    return L"";
}

const unordered_map<string, MetaStore::Creator>& MetaStore::InitAndGetCreators()
{
    if (_creators.size() == 0)
    {
        _creators[".fbx"] = []() { return make_unique<ModelMeta>(); };

        {
            function<unique_ptr<MetaFile>()> texCreator = []() { return make_unique<TextureMeta>(); };
            _creators[".png"] = texCreator;
            _creators[".tga"] = texCreator;
            _creators[".jpg"] = texCreator;
            _creators[".jpeg"] = texCreator;
            _creators[".bmp"] = texCreator;
        }
    }

    return _creators;
}
