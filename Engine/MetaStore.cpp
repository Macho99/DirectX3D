#include "pch.h"
#include "MetaStore.h"
#include "fstream"
#include "cereal/types/polymorphic.hpp"
#include "cereal/archives/json.hpp"
#include "ModelMeta.h"
#include "TextureMeta.h"
#include "FolderMeta.h"
#include "NotSupportMeta.h"
#include "AnimationMeta.h"
#include "MeshMeta.h"
#include "MaterialMeta.h"

unordered_map<string, MetaStore::Creator> MetaStore::_creators;

fs::path MetaStore::MetaPathForSource(const fs::path& sourceAbs)
{
    return fs::path(sourceAbs.wstring() + L".meta");
}

fs::path MetaStore::SourcePathForMeta(const fs::path& metaAbs)
{
    fs::path src = metaAbs;
    if (src.extension() == L".meta")
        src.replace_extension();
    return src;
}

unique_ptr<MetaFile> MetaStore::TryLoad(const fs::path& metaAbs)
{
    unique_ptr<MetaFile> meta;
    const wstring sourceAbs = SourcePathForMeta(metaAbs);
    try
    {
        std::ifstream is(metaAbs);
        cereal::JSONInputArchive archive(is);
        archive(meta);
    }
    catch (const std::exception& e)
    {
        DBG->LogW(L"[MetaStore] TryLoad failed And Recreated: " + metaAbs.wstring() + L", error: " + Utils::ToWString(e.what()));
        meta = Create(sourceAbs);
    }

    meta->_absPath = sourceAbs;
    ImportIfDirty(meta);

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
        if (fs::is_directory(sourceAbs))
            meta = make_unique<FolderMeta>();
        else
            meta = make_unique<NotSupportMeta>();
    }

    AssetId assetId;
    if (RESOURCES->GetAssetDatabase().TryGetReservedAssetId(sourceAbs, assetId))
    {
        DBG->LogW(L"[MetaStore] Create with reserved AssetId: " + sourceAbs.wstring() + L", AssetId: " + assetId.ToWString());
    }
    else
    {
        assetId = AssetId::CreateAssetId();
    }

    meta->_assetId = assetId;
    meta->_absPath = sourceAbs;

    ImportIfDirty(meta);
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

void MetaStore::ImportIfDirty(unique_ptr<MetaFile>& metaFile)
{
    bool imported = metaFile->ImportIfDirty();
    if (imported == false)
        return;

    std::ofstream os(MetaPathForSource(metaFile->_absPath));
    cereal::JSONOutputArchive archive(os);
    archive(metaFile);
}

//wstring MetaStore::GetResourceExtension(ResourceType resourceType)
//{
//    switch (resourceType)
//    {
//        case ResourceType::Mesh:        return L".mesh";
//        case ResourceType::Material:    return L".mat";
//        case ResourceType::Shader:      return L".fx";
//        case ResourceType::Animation:   return L".clip";
//    }
//
//    assert(resourceType == ResourceType::Texture, "Get Texture Extension Not Support");
//    assert(false, "GetResourceExtension: Unknown resource type");
//    return L"";
//}

const unordered_map<string, MetaStore::Creator>& MetaStore::InitAndGetCreators()
{
    if (_creators.size() == 0)
    {
        _creators[".fbx"] = []() { return make_unique<ModelMeta>(); };
        _creators[".clip"] = []() { return make_unique<AnimationMeta>(); };
        _creators[".mesh"] = []() { return make_unique<MeshMeta>(); };
        _creators[".mat"] = []() { return make_unique<MaterialMeta>(); };

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
