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
    isReadOnly = true;
    changed |= Super::OnGUI(isReadOnly);
    changed |= OnGUIUtils::DrawResourceRef("Mesh", _mesh, isReadOnly);
    for (int i = 0; i < _materials.size(); i++)
    {
        string label = "Material " + to_string(i);
        changed |= OnGUIUtils::DrawResourceRef(label.c_str(), _materials[i], isReadOnly);
    }
    for (int i = 0; i < _animations.size(); i++)
    {
        string label = "Animation " + to_string(i);
        changed |= OnGUIUtils::DrawResourceRef(label.c_str(), _animations[i], isReadOnly);
    }
    return changed;
}
