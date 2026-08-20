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
#include "../NavBuild/NavFileUtils.h"

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
	InvalidateAnimationTexture();

    ModelMeshResource* mesh = _mesh.Resolve();
    if (mesh == nullptr)
		return;
    mesh->BindCacheInfo(_materials);
}

bool Model::EnsureAnimationTexture()
{
	if (_animations.empty())
	{
		InvalidateAnimationTexture();
		return false;
	}

	vector<AssetId> animationAssetIds;
	animationAssetIds.reserve(_animations.size());
	for (const ResourceRef<ModelAnimation>& animationRef : _animations)
		animationAssetIds.push_back(animationRef.GetAssetId());

	if (_animationTransformTexture != nullptr && _animationTransformSRV != nullptr &&
		_animTransforms.size() == _animations.size() && _animationCacheAssetIds == animationAssetIds)
	{
		return true;
	}

	InvalidateAnimationTexture();
	ModelMeshResource* mesh = GetMesh();
	if (mesh == nullptr || mesh->GetBoneCount() > MAX_MODEL_TRANSFORMS)
		return false;

	for (uint32 index = 0; index < GetAnimationCount(); ++index)
	{
		ModelAnimation* animation = GetAnimationByIndex(index);
		if (animation == nullptr || animation->GetFrameCount() > MAX_MODEL_KEYFRAMES)
			return false;
	}

	_animTransforms.resize(GetAnimationCount());
	for (uint32 index = 0; index < GetAnimationCount(); ++index)
		CreateAnimationTransform(index);

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = MAX_MODEL_TRANSFORMS * 4;
	textureDesc.Height = MAX_MODEL_KEYFRAMES;
	textureDesc.ArraySize = GetAnimationCount();
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MipLevels = 1;
	textureDesc.SampleDesc.Count = 1;

	const uint32 rowSize = MAX_MODEL_TRANSFORMS * sizeof(Matrix);
	const uint32 pageSize = rowSize * MAX_MODEL_KEYFRAMES;
	vector<BYTE> textureData(static_cast<size_t>(pageSize) * GetAnimationCount());
	for (uint32 animationIndex = 0; animationIndex < GetAnimationCount(); ++animationIndex)
	{
		BYTE* page = textureData.data() + static_cast<size_t>(animationIndex) * pageSize;
		for (uint32 frameIndex = 0; frameIndex < MAX_MODEL_KEYFRAMES; ++frameIndex)
		{
			::memcpy(page + static_cast<size_t>(rowSize) * frameIndex,
				_animTransforms[animationIndex].transforms[frameIndex].data(), rowSize);
		}
	}

	vector<D3D11_SUBRESOURCE_DATA> subResources(GetAnimationCount());
	for (uint32 animationIndex = 0; animationIndex < GetAnimationCount(); ++animationIndex)
	{
		subResources[animationIndex].pSysMem = textureData.data() + static_cast<size_t>(animationIndex) * pageSize;
		subResources[animationIndex].SysMemPitch = rowSize;
		subResources[animationIndex].SysMemSlicePitch = pageSize;
	}
	DX_CREATE_TEXTURE2D(&textureDesc, subResources.data(), _animationTransformTexture);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.ArraySize = GetAnimationCount();
	DX_CREATE_SRV(_animationTransformTexture.Get(), &srvDesc, _animationTransformSRV);

	_animationCacheAssetIds = move(animationAssetIds);
	return true;
}

void Model::InvalidateAnimationTexture()
{
	_animTransforms.clear();
	_animationCacheAssetIds.clear();
	_animationTransformSRV.Reset();
	_animationTransformTexture.Reset();
}

void Model::ExtractAnimationRootPositionsAndEvents(const fs::path path)
{
	NavFileUtils fileUtils;
	fileUtils.Open(path.wstring(), NavFileMode::Write);

	const bool hasAnimationTransforms = EnsureAnimationTexture();
	fileUtils.Write<uint32>(GetAnimationCount());

	for (uint32 animationIndex = 0; animationIndex < GetAnimationCount(); ++animationIndex)
	{
		ModelAnimation* animation = GetAnimationByIndex(animationIndex);
		const string animationName = animation != nullptr ? Utils::ToString(animation->GetName()) : string();
		fileUtils.Write(animationName);

		const uint32 frameCount = animation != nullptr ? animation->GetFrameCount() : 0;
		fileUtils.Write(frameCount);
		for (uint32 frameIndex = 0; frameIndex < frameCount; ++frameIndex)
		{
			Vec3 rootPosition = Vec3::Zero;
			if (hasAnimationTransforms && animationIndex < _animTransforms.size() && frameIndex < MAX_MODEL_KEYFRAMES)
			{
				Vec3 scale;
				Quaternion rotation;
				_animTransforms[animationIndex].rootTransforms[frameIndex].Decompose(scale, rotation, rootPosition);
			}
			fileUtils.Write(rootPosition);
		}

		const vector<AnimationEvent> emptyEvents;
		const vector<AnimationEvent>& animationEvents = animation != nullptr ? animation->GetAnimationEvents() : emptyEvents;
		fileUtils.Write<uint32>(static_cast<uint32>(animationEvents.size()));
		for (const AnimationEvent& animationEvent : animationEvents)
		{
			fileUtils.Write(animationEvent.eventName);
            fileUtils.Write(animationEvent.boolParam);
            fileUtils.Write(animationEvent.intParam);
            fileUtils.Write(animationEvent.floatParam);
            fileUtils.Write(animationEvent.frame);
		}
	}
}

void Model::CreateAnimationTransform(uint32 index)
{
	ModelMeshResource* mesh = GetMesh();
	ModelAnimation* animation = GetAnimationByIndex(index);
	if (mesh == nullptr || animation == nullptr || index >= _animTransforms.size())
		return;

	vector<Matrix> boneTransforms(MAX_MODEL_TRANSFORMS, Matrix::Identity);
	for (uint32 frameIndex = 0; frameIndex < animation->GetFrameCount(); ++frameIndex)
	{
		for (uint32 boneIndex = 0; boneIndex < mesh->GetBoneCount(); ++boneIndex)
		{
			shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(boneIndex);
			Matrix animationMatrix = bone->localMatrix;
			shared_ptr<ModelKeyframe> frame = animation->GetKeyframe(bone->name);
			if (frame != nullptr && frameIndex < frame->transforms.size())
			{
				const ModelKeyframeData& data = frame->transforms[frameIndex];
				animationMatrix = Matrix::CreateScale(data.scale)
					* Matrix::CreateFromQuaternion(data.rotation)
					* Matrix::CreateTranslation(data.translation);
			}

			Matrix parentMatrix = Matrix::Identity;
			if (bone->parentIndex >= 0)
			{
				parentMatrix = boneTransforms[bone->parentIndex];
			}
			else
			{
				const AnimationClipImportSetting& importSetting = animation->GetAnimationClipImportSetting();
				if (importSetting.extractRootMotion)
				{
					Vec3 scale;
					Vec3 position;
					Quaternion rotation;
					animationMatrix.Decompose(scale, rotation, position);
					const Vec3 verticalPosition(0.f, position.y, 0.f);
					animationMatrix = Matrix::CreateScale(scale)
						* Matrix::CreateFromQuaternion(rotation)
						* Matrix::CreateTranslation(verticalPosition);
					position.y = 0.f;
					_animTransforms[index].rootTransforms[frameIndex] = Matrix::CreateTranslation(position);
				}
			}

			boneTransforms[boneIndex] = animationMatrix * parentMatrix;
			_animTransforms[index].transforms[frameIndex][boneIndex] = bone->offsetMatrix * boneTransforms[boneIndex];
		}
	}
}

bool Model::HasValidRenderResources() const
{
	ModelMeshResource* mesh = _mesh.Resolve();
	if (mesh == nullptr || mesh->GetMeshes().empty() || _materials.empty())
		return false;

	for (const ResourceRef<Material>& materialRef : _materials)
	{
		if (materialRef.Resolve() == nullptr)
			return false;
	}

	for (const shared_ptr<ModelMesh>& modelMesh : mesh->GetMeshes())
	{
		if (modelMesh == nullptr || modelMesh->material.Resolve() == nullptr)
			return false;
	}

	return true;
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
	bool animationCacheChanged = false;
    changed |= Super::OnGUI(isReadOnly);
    animationCacheChanged |= OnGUIUtils::DrawResourceRef("Mesh", _mesh, isReadOnly);
    changed |= animationCacheChanged;
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
			animationCacheChanged = true;
        }
    }

    for (int i = 0; i < _animations.size(); i++)
    {
        string label = "Animation " + to_string(i);
		const bool animationChanged = OnGUIUtils::DrawResourceRef(label.c_str(), _animations[i], isReadOnly);
		changed |= animationChanged;
		animationCacheChanged |= animationChanged;
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
		animationCacheChanged = true;
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

	if (animationCacheChanged)
		InvalidateAnimationTexture();

    return changed;
}

void Model::OnMenu(bool isReadOnly)
{
    if (ImGui::MenuItem("Extract Animation Root Positions and Events"))
    {
		fs::path outputPath = FileUtils::SaveFileDialog(
			L"Animation Root And Event Data",
			L"Animation Root And Event Data (*.animData)\0*.animData\0All Files (*.*)\0*.*\0",
			L"animData",
			L"..\\..\\");
		if (!outputPath.empty())
			ExtractAnimationRootPositionsAndEvents(outputPath);
    }
}
