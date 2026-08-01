#include "pch.h"
#include "Model.h"
#include "Utils.h"
#include "FileUtils.h"
#include "tinyxml2.h"
#include <filesystem>
#include "Material.h"
#include "ModelMesh.h"
#include "ModelAnimation.h"
#include "OnGUIUtils.h"

Model::Model()
    : Super(StaticType)
{

}

Model::Model(vector<ResourceRef<Material>> materials, ResourceRef<ModelMeshResource> mesh, vector<ResourceRef<ModelAnimation>> animations)
    :Super(StaticType), _materials(materials), _mesh(mesh), _animations(animations)
{
	BindCache();
}

Model::~Model()
{

}

void Model::BindCache()
{
    ModelMeshResource* mesh = _mesh.Resolve();
    if (mesh == nullptr)
		return;
    mesh->BindCacheInfo(_materials);
}

ResourceRef<Material> Model::GetMaterialByName(const wstring& name)
{
	for (auto& materialRef : _materials)
	{
        Material* material = materialRef.Resolve();

		if (material->GetName() == name)
			return materialRef;
	}

	return MaterialRef();
}

ModelAnimation* Model::GetAnimationByIndex(UINT index)
{
    if (index < 0 || index >= _animations.size())
        return nullptr;
    return _animations[index].Resolve();
}

ModelAnimation* Model::GetAnimationByName(wstring name)
{
	for (ResourceRef<ModelAnimation>& animationRef : _animations)
	{
        ModelAnimation* anim = animationRef.Resolve();
		if (anim->GetName() == name)
			return anim;
	}

	return nullptr;
}

bool Model::OnGUI(bool isReadOnly)
{
	bool changed = false;
    changed |= Super::OnGUI(isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Mesh", _mesh, isReadOnly);
    ImGui::Separator();
    if (isReadOnly == false)
    {
        uint32 materialCount = _materials.size();
        bool materialCountChanged = OnGUIUtils::DrawUInt32("Material Count", &materialCount, 1.f, isReadOnly);
        if (materialCountChanged && materialCount >= 0)
        {
            if (materialCount > _materials.size())
            {
                for (int i = _materials.size(); i < materialCount; i++)
                {
                    _materials.push_back(ResourceRef<Material>());
                }
            }
            else
            {
                _materials.resize(materialCount);
            }
            changed = true;
        }
    }

    for (int i = 0; i < _materials.size(); i++)
    {
        string label = "Material " + to_string(i);
        changed |= OnGUIUtils::DrawResourceRef(label.c_str(), _materials[i], isReadOnly);
    }
    ImGui::Separator();

    if (isReadOnly == false)
    {
        int animationCount = _animations.size();
        bool animationCountChanged = OnGUIUtils::DrawInt32("Animation Count", &animationCount, 1.f, isReadOnly);
        if (animationCountChanged && animationCount >= 0)
        {
            if (animationCount > _animations.size())
            {
                for (int i = _animations.size(); i < animationCount; i++)
                {
                    _animations.push_back(ResourceRef<ModelAnimation>());
                }
            }
            else
            {
                _animations.resize(animationCount);
            }
            changed = true;
        }
    }

    for (int i = 0; i < _animations.size(); i++)
    {
        string label = "Animation " + to_string(i);
        changed |= OnGUIUtils::DrawResourceRef(label.c_str(), _animations[i], isReadOnly);
    }

    if (ImGui::Button("Auto Animation Setting"))
    {
        _animations.clear();
        fs::path folderPath(_path);
        folderPath = folderPath.parent_path();

        vector<fs::path> filePaths;
        for (const auto& entry : fs::directory_iterator(folderPath))
        {
            if (entry.is_regular_file())
                filePaths.push_back(entry.path());
        }

        for (const fs::path& filePath : filePaths)
        {
            if (_wcsicmp(filePath.extension().c_str(), L".clip") == 0)
            {
                _animations.push_back(
                    RESOURCES->GetResourceRefByAbsPath<ModelAnimation>(filePath));
                continue;
            }

            if (_wcsicmp(filePath.extension().c_str(), L".fbx") != 0)
                continue;

            const bool hasSameNameClip = any_of(
                filePaths.begin(), filePaths.end(),
                [&filePath](const fs::path& otherPath)
                {
                    return _wcsicmp(otherPath.extension().c_str(), L".clip") == 0
                        && _wcsicmp(otherPath.stem().c_str(), filePath.stem().c_str()) == 0;
                });

            if (hasSameNameClip)
                continue;

            Model* model = RESOURCES->GetResourceRefByAbsPath<Model>(filePath).Resolve();
            if (model == nullptr)
                continue;

            const vector<ResourceRef<ModelAnimation>>& animations = model->GetAnimations();
            for (const ResourceRef<ModelAnimation>& animRef : animations)
            {
                _animations.push_back(animRef);
            }
        }

        changed = true;
    }


    return changed;
}
