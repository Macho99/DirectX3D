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
#include "AnimationOverrideMeta.h"

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

int Model::GetVersion() const
{
    return 2;
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

int32 Model::GetAnimationIndexByName(wstring name) const
{
    for (int32 i = 0; i < _animations.size(); ++i)
    {
        ModelAnimation* anim = _animations[i].Resolve();
        if (anim != nullptr && anim->GetName() == name)
            return i;
    }
    return -1;
}

ModelSocket* Model::GetModelSocketByName(const string& name)
{
	auto it = find_if(_modelSockets.begin(), _modelSockets.end(),
		[&](const ModelSocket& socket) { return socket.name == name; });
	return it != _modelSockets.end() ? &(*it) : nullptr;
}

const ModelSocket* Model::GetModelSocketByName(const string& name) const
{
	auto it = find_if(_modelSockets.begin(), _modelSockets.end(),
		[&](const ModelSocket& socket) { return socket.name == name; });
	return it != _modelSockets.end() ? &(*it) : nullptr;
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

        vector<wstring> priorityExtensions = { L".clip", Utils::ToWString(AnimationOverrideMeta::GetExtension()) };

        for (const fs::path& filePath : filePaths)
        {
            const wstring extension = filePath.extension().wstring();
            bool isPriorityExtension = false;
            for (const wstring& priorityExt : priorityExtensions)
            {
                if (extension == priorityExt)
                {
                    _animations.push_back(
                        RESOURCES->GetResourceRefByAbsPath<ModelAnimation>(filePath));
                    isPriorityExtension = true;
                }
            }
            if (isPriorityExtension)
                continue;

            if (extension != L".fbx")
                continue;

            const bool hasSameNameClip = any_of(
                filePaths.begin(), filePaths.end(),
                [&](const fs::path& otherPath)
                {
                    for (const wstring& priorityExt : priorityExtensions)
                    {
                        if (otherPath.extension() == priorityExt.c_str()
                            && otherPath.stem() == filePath.stem())
                        {
                            return true;
                        }
                    }
                    return false;
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

    ImGui::SeparatorText("Model Sockets");
    ModelMeshResource* mesh = GetMesh();
    if (!isReadOnly && ImGui::Button("Add Model Socket"))
    {
        ModelSocket socket;
        const string baseName = socket.name;
        int suffix = 1;
        while (GetModelSocketByName(socket.name) != nullptr)
            socket.name = baseName + to_string(suffix++);
        _modelSockets.push_back(socket);
        changed = true;
    }

    int removeSocketIndex = -1;
    for (int i = 0; i < static_cast<int>(_modelSockets.size()); ++i)
    {
        ModelSocket& socket = _modelSockets[i];
        ImGui::PushID(i);
        const string header = socket.name.empty() ? "Unnamed Socket" : socket.name;
        if (ImGui::TreeNodeEx("Socket", ImGuiTreeNodeFlags_DefaultOpen, "%d : %s", i, header.c_str()))
        {
            OnGUIUtils::DrawString("Name", &socket.name, isReadOnly);

            string bonePreview = "Unassigned";
            if (mesh != nullptr && socket.boneIndex >= 0 && socket.boneIndex < static_cast<int32>(mesh->GetBoneCount()))
                bonePreview = to_string(socket.boneIndex) + " : " + Utils::ToString(mesh->GetBoneByIndex(socket.boneIndex)->name);
            else if (socket.boneIndex >= 0)
                bonePreview = "Missing bone (" + to_string(socket.boneIndex) + ")";

            if (isReadOnly)
                ImGui::BeginDisabled();
            if (ImGui::BeginCombo("Bone", bonePreview.c_str()))
            {
                if (ImGui::Selectable("Unassigned", socket.boneIndex < 0))
                {
                    socket.boneIndex = -1;
                    changed = true;
                }
                if (mesh != nullptr)
                {
                    for (uint32 boneIndex = 0; boneIndex < mesh->GetBoneCount(); ++boneIndex)
                    {
                        const string boneLabel = to_string(boneIndex) + " : " + Utils::ToString(mesh->GetBoneByIndex(boneIndex)->name);
                        if (ImGui::Selectable(boneLabel.c_str(), socket.boneIndex == static_cast<int32>(boneIndex)))
                        {
                            socket.boneIndex = static_cast<int32>(boneIndex);
                            changed = true;
                        }
                    }
                }
                ImGui::EndCombo();
            }
            if (isReadOnly)
                ImGui::EndDisabled();

            if (!isReadOnly && ImGui::Button("Remove Socket"))
                removeSocketIndex = i;
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (removeSocketIndex >= 0)
    {
        _modelSockets.erase(_modelSockets.begin() + removeSocketIndex);
        changed = true;
    }

    return changed;
}
