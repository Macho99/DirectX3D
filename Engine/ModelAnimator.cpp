#include "pch.h"
#include "ModelAnimator.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "ModelAnimation.h"
#include "Camera.h"
#include "Light.h"
#include "OnGUIUtils.h"
#include "MathUtils.h"
#include "SceneView.h"

ModelAnimator::ModelAnimator()
	: Super(StaticType)
{
	_tweenDesc.tweenSumTime += rand();
	_tweenDesc.speed = (rand() % 50) / 10.0f + 1.0f;

    _pass = 2;
	_shader = RESOURCES->GetDefaultShader();
}

ModelAnimator::~ModelAnimator()
{
}

void ModelAnimator::Awake()
{
    static int awakeCount = 0;
	if (awakeCount++ != 0)
		return;
    Transform* transform = GetTransform();
    transform->SetLocalScale(Vec3(0.01f));
	_skinnedMesh.LoadMesh("D:\\Projects\\source\\repos\\GameCoding\\Assets\\Models\\Paladin\\Paladin WProp J Nordstrom.fbx");
}

void ModelAnimator::Update()
{
	UpdateTweenData();
}

void ModelAnimator::SetShader(ResourceRef<Shader> shader)
{
    _shader = shader;
}

void ModelAnimator::SetModel(ResourceRef<Model> model)
{
	_model = model;

    Model* modelPtr = _model.Resolve();
    if (modelPtr == nullptr)
        return;
	int animCount = modelPtr->GetAnimationCount();
	_tweenDesc.next.animIndex = rand() % animCount;
}

void ModelAnimator::RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech)
{
    Model* model = _model.Resolve();
	if (model == nullptr)
		return;

    Shader* shader = _shader.Resolve();
    if (shader == nullptr)
        return;

    ModelMeshResource* mesh = model->GetMesh();
    if (mesh == nullptr)
        return;

	if (_texture == nullptr)
		CreateTexture();

	if (Super::Render(renderTech) == false)
		return;

	//// GlobalData
	//_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	//
	//// Light
	//auto lightObj = SCENE->GetCurrentScene()->GetLight();
	//if (lightObj)
	//	_shader->PushLightData(lightObj->GetLight()->GetLightDesc());

	// SRV를 통해 정보 전달
	_shader.Resolve()->GetSRV("TransformMap")->SetResource(_srv.Get());

	// Bone
	BoneDesc boneDesc;

	const uint32 boneCount = mesh->GetBoneCount();
	if (_showAnimationDebug)
	{

	}
	else
	{
		_skinnedMesh.LoadBoneInfos(TIME->GetGameTime());
	}
	vector<SkinnedMesh::BoneInfo>& boneInfos = _skinnedMesh.GetBoneInfos();
    for (uint32 i = 0; i < boneCount; i++)
    {
		boneDesc.transforms[i] = boneInfos[i].FinalTransformation;
    }
	//for (uint32 i = 0; i < boneCount; i++)
	//{
	//	shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(i);
	//	boneDesc.transforms[i] = bone->transform;
	//}
	shader->PushBoneData(boneDesc);

	const vector<shared_ptr<ModelMesh>>& meshes = mesh->GetMeshes();
	for (const shared_ptr<ModelMesh>& modelMesh : meshes)
	{
        Material* material = modelMesh->material.Resolve();
		if (material)
			material->Update();

		shader->GetScalar("BoneIndex")->SetInt(modelMesh->boneIndex);

		modelMesh->vertexBuffer->PushData();
		modelMesh->indexBuffer->PushData();
		buffer->PushData();
		shader->DrawIndexedInstanced(renderTech, _pass, modelMesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelAnimator::GetInstanceID()
{
	return make_pair((uint64)_model.GetAssetId().GetLeftId(), (uint64)_shader.GetAssetId().GetLeftId());
}

bool ModelAnimator::OnGUI()
{
	bool changed = false;
    changed |= Super::OnGUI();
	ImGui::Separator();
    changed |= OnGUIUtils::DrawResourceRef("Model", _model);
    changed |= OnGUIUtils::DrawFloat("Tween Duration", &_tweenDesc.tweenDuration, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Tween Ratio", &_tweenDesc.tweenRatio, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Tween SumTime", &_tweenDesc.tweenSumTime, 0.01f);
	changed |= OnGUIUtils::DrawInt32("Cur Anim Index", &_tweenDesc.cur.animIndex, 1.f);
	changed |= OnGUIUtils::DrawInt32("Next Anim Index", &_tweenDesc.next.animIndex, 1.f);
	changed |= OnGUIUtils::DrawFloat("Anim Speed", &_tweenDesc.speed, 0.1f);
	bool showAnimDebugChanged = OnGUIUtils::DrawBool("Show Animation Debug", &_showAnimationDebug);
    changed |= showAnimDebugChanged;
	if (showAnimDebugChanged && _showAnimationDebug == false)
	{
        EDITOR->GetSceneView()->ClearTransformGizmoOverride();
	}

	if (_showAnimationDebug)
		DrawDebugWindow();

	return changed;
}

bool ModelAnimator::TryInitialize()
{
	if(_initialized)
        return true;

    Model* modelPtr = _model.Resolve();
    if (modelPtr == nullptr)
        return false;

    Shader* shaderPtr = _shader.Resolve();
    if (shaderPtr == nullptr)
        return false;

	auto& materials = modelPtr->GetMaterials();
	for (auto& material : materials)
	{
		material.Resolve()->SetShader(_shader);
		_material = material;
		break;
	}

    _initialized = true;
    return true;
}

void ModelAnimator::CreateTexture()
{
    Model* model = _model.Resolve();
	if (model == nullptr)
		return;

	if (model->GetAnimationCount() == 0)
		return;

	_animTransforms.resize(model->GetAnimationCount());

	for (uint32 i = 0; i < model->GetAnimationCount(); i++)
	{
		CreateAnimationTransform(i);
	}

	// Creature Texture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = MAX_MODEL_TRANSFORMS * 4;
		desc.Height = MAX_MODEL_KEYFRAMES;
		desc.ArraySize = model->GetAnimationCount();
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // 16바이트
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;

		const uint32 dataSize = MAX_MODEL_TRANSFORMS * sizeof(Matrix);
		const uint32 pageSize = dataSize * MAX_MODEL_KEYFRAMES;
		void* mallocPtr = ::malloc(pageSize * model->GetAnimationCount());

		// 파편화된 데이터를 조립한다.
		for (uint32 c = 0; c < model->GetAnimationCount(); c++)
		{
			uint32 startOffset = c * pageSize;

			BYTE* pageStartPtr = reinterpret_cast<BYTE*>(mallocPtr) + startOffset;

			for (uint32 f = 0; f < MAX_MODEL_KEYFRAMES; f++)
			{
				void* ptr = pageStartPtr + dataSize * f;
				::memcpy(ptr, _animTransforms[c].transforms[f].data(), dataSize);
			}
		}

		// 리소스 만들기
		vector<D3D11_SUBRESOURCE_DATA> subResources(model->GetAnimationCount());

		for (uint32 c = 0; c < model->GetAnimationCount(); c++)
		{
			void* ptr = (BYTE*)mallocPtr + c * pageSize;
			subResources[c].pSysMem = ptr;
			subResources[c].SysMemPitch = dataSize;
			subResources[c].SysMemSlicePitch = pageSize;
		}

		DX_CREATE_TEXTURE2D(&desc, subResources.data(), _texture);

		::free(mallocPtr);
	}

	// Create SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipLevels = 1;
		desc.Texture2DArray.ArraySize = model->GetAnimationCount();

		DX_CREATE_SRV(_texture.Get(), &desc, _srv);
	}
}

void ModelAnimator::CreateAnimationTransform(uint32 index)
{
    Model* model = _model.Resolve();
    ModelMeshResource* mesh = model->GetMesh();

	vector<Matrix> tempAnimBoneTransforms(MAX_MODEL_TRANSFORMS, Matrix::Identity);
	ModelAnimation* animation = model->GetAnimationByIndex(index);

	for (uint32 f = 0; f < animation->GetFrameCount(); f++)
	{
		for (uint32 b = 0; b < mesh->GetBoneCount(); b++)
		{
			//shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(b);
			//
			//Matrix matAnim;
			//
			//shared_ptr<ModelKeyframe> frame = animation->GetKeyframe(bone->name);
			//if (frame != nullptr)
			//{
			//	ModelKeyframeData& data = frame->transforms[f];
			//	Matrix S, R, T;
			//	S = Matrix::CreateScale(data.scale);
			//	R = Matrix::CreateFromQuaternion(data.rotation);
			//	T = Matrix::CreateTranslation(data.translation);
			//
			//	matAnim = S * R * T;
			//}
			//else
			//{
			//	matAnim = Matrix::Identity;
			//}
			//
			//Matrix toRootMat = bone->transform;
			//Matrix invGlobal = toRootMat.Invert();
			//
			//int32 parentIndex = bone->parentIndex;
			//
			//Matrix matParent = Matrix::Identity;
			//if (parentIndex >= 0)
			//{
			//	matParent = tempAnimBoneTransforms[parentIndex];
			//}
			//
			//tempAnimBoneTransforms[b] = matAnim * matParent;
			//_animTransforms[index].transforms[f][b] = invGlobal * tempAnimBoneTransforms[b];
		}
	}
}

void ModelAnimator::UpdateTweenData()
{
    Model* model = _model.Resolve();
    if (model == nullptr)
        return;

	TweenDesc& desc = _tweenDesc;

	desc.cur.sumTime += DT;
	// 현재 애니메이션
	{
		ModelAnimation* currentAnim = model->GetAnimationByIndex(desc.cur.animIndex);
		if (currentAnim)
		{
			float timePerFrame = 1 / (currentAnim->GetFrameRate() * desc.speed);
			if (desc.cur.sumTime >= timePerFrame)
			{
				desc.cur.sumTime = 0;
				desc.cur.curFrame = (desc.cur.curFrame + 1) % currentAnim->GetFrameCount();
				desc.cur.nextFrame = (desc.cur.curFrame + 1) % currentAnim->GetFrameCount();
			}

			desc.cur.ratio = (desc.cur.sumTime / timePerFrame);
		}
	}

	// 다음 애니메이션이 있다면
	if (desc.next.animIndex >= 0)
	{
		desc.tweenSumTime += DT;
		desc.tweenRatio = desc.tweenSumTime / desc.tweenDuration;

		if (desc.tweenRatio >= 1.f)
		{
			desc.cur = desc.next;
			desc.ClearNextAnim();
		}
		else
		{
			// 교체중
			desc.next.sumTime += DT;

			desc.next.animIndex = std::clamp(desc.next.animIndex, 0, (int)model->GetAnimationCount() - 1);
			ModelAnimation* nextAnim = model->GetAnimationByIndex(desc.next.animIndex);
			float timePerFrame = 1.f / (nextAnim->GetFrameCount() * desc.speed);

			if (desc.next.ratio >= 1.f)
			{
				desc.next.sumTime = 0;

				desc.next.curFrame = (desc.next.curFrame + 1) % nextAnim->GetFrameCount();
				desc.next.nextFrame = (desc.next.curFrame + 1) % nextAnim->GetFrameCount();
			}

			desc.next.ratio = desc.next.sumTime / timePerFrame;
		}
	}
}

void ModelAnimator::DrawDebugWindow()
{
	string objectName = GetGameObject() ? GetGameObject()->GetName() : "ModelAnimator";
	string windowTitle = "Animation Debug - " + objectName + "##ModelAnimator_" + to_string((uintptr_t)this);
	ImGui::SetNextWindowSize(ImVec2(1050.0f, 700.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(windowTitle.c_str(), &_showAnimationDebug))
	{
		ImGui::End();
		return;
	}

	const uint32 animationCount = _skinnedMesh.GetAnimationCount();
	if (animationCount == 0)
	{
		ImGui::TextDisabled("No animation is loaded in SkinnedMesh.");
		ImGui::End();
		return;
	}

	_debugAnimationIndex = clamp(_debugAnimationIndex, 0, (int)animationCount - 1);
	string animationName = _skinnedMesh.GetAnimationName(_debugAnimationIndex);
	if (ImGui::BeginCombo("Animation", animationName.c_str()))
	{
		for (uint32 i = 0; i < animationCount; ++i)
		{
			const bool selected = i == (uint32)_debugAnimationIndex;
			string name = _skinnedMesh.GetAnimationName(i);
			if (ImGui::Selectable(name.c_str(), selected))
			{
				_debugAnimationIndex = (int)i;
				_debugFrameIndex = 0;
				_debugBoneIndex = 0;
                _lastDebugFrameIndex = -1;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	const uint32 frameCount = _skinnedMesh.GetAnimationFrameCount(_debugAnimationIndex);
	_debugFrameIndex = clamp(_debugFrameIndex, 0, max(0, (int)frameCount - 1));
	ImGui::SetNextItemWidth(500.0f);
	ImGui::SliderInt("Frame", &_debugFrameIndex, 0, max(0, (int)frameCount - 1));

	const float ticksPerSecond = _skinnedMesh.GetAnimationTicksPerSecond(_debugAnimationIndex);
	const float timeSeconds = ticksPerSecond > 0.0f ? _debugFrameIndex / ticksPerSecond : 0.0f;

    if (_debugFrameIndex != _lastDebugFrameIndex)
		_skinnedMesh.LoadBoneInfos(timeSeconds, _debugAnimationIndex);
    _lastDebugFrameIndex = _debugFrameIndex;

	vector<SkinnedMesh::BoneInfo>& boneInfos = _skinnedMesh.GetBoneInfos();

	ImGui::SameLine();
	ImGui::Text("tick %d / %.3f | %.3fs | %.2f ticks/s",
		_debugFrameIndex,
		_skinnedMesh.GetAnimationDuration(_debugAnimationIndex),
		timeSeconds,
		ticksPerSecond);
	ImGui::Separator();

	const float listWidth = 330.0f;
	ImGui::BeginChild("BoneList", ImVec2(listWidth, 0.0f), true);
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::Separator();

	function<void(int)> DrawBoneTree = [&](int boneIndex)
		{
			if (boneIndex < 0 || boneIndex >= (int)boneInfos.size())
				return;

			const SkinnedMesh::BoneInfo& boneInfo = boneInfos[boneIndex];
			const bool hasChildren = !boneInfo.ChildrenIndices.empty();
			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (_debugBoneIndex == boneIndex)
				flags |= ImGuiTreeNodeFlags_Selected;
			if (!hasChildren)
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

			const string boneName = _skinnedMesh.GetBoneName(boneIndex);
			const bool open = ImGui::TreeNodeEx(
				(void*)(intptr_t)boneIndex, flags, "%s  [%d]", boneName.c_str(), boneIndex);
			if (ImGui::IsItemClicked())
				_debugBoneIndex = boneIndex;

			if (hasChildren && open)
			{
				for (int childIndex : boneInfo.ChildrenIndices)
					DrawBoneTree(childIndex);
				ImGui::TreePop();
			}
		};

	for (int boneIndex = 0; boneIndex < (int)boneInfos.size(); ++boneIndex)
	{
		if (boneInfos[boneIndex].ParentIndex < 0)
			DrawBoneTree(boneIndex);
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("BoneDetails", ImVec2(0.0f, 0.0f), true);
	if (!boneInfos.empty())
	{
		_debugBoneIndex = clamp(_debugBoneIndex, 0, (int)boneInfos.size() - 1);
		SkinnedMesh::BoneInfo& boneInfo = boneInfos[_debugBoneIndex];
		const string boneName = _skinnedMesh.GetBoneName(_debugBoneIndex);
		const string parentName = boneInfo.ParentIndex >= 0
			? _skinnedMesh.GetBoneName(boneInfo.ParentIndex)
			: "<root>";

		ImGui::Text("%s  [%d]", boneName.c_str(), _debugBoneIndex);
		ImGui::TextDisabled("Parent: %s [%d] | Children: %u",
			parentName.c_str(),
			boneInfo.ParentIndex,
			(uint32)boneInfo.ChildrenIndices.size());
		ImGui::SeparatorText("Bone transforms");
		ImGui::TextDisabled("BoneLocal = Global * ParentGlobalInverse");
		ImGui::TextDisabled("Final = Offset * Global * GlobalInverseRoot");

		Vec3 position, eulerRotation, scale;
        Quaternion rotation;
        boneInfo.BoneLocalTransform.Decompose(scale, rotation, position);
		eulerRotation = Transform::ToEulerAngles(rotation);
        eulerRotation = MathUtils::RadToDeg(eulerRotation);

        bool changed = false;

		changed |= OnGUIUtils::DrawVec3("Position", &position, 0.1f, false);
		changed |= OnGUIUtils::DrawVec3("Rotation", &eulerRotation, 0.1f, false);
		changed |= OnGUIUtils::DrawVec3("Scale", &scale, 0.1f, false);
        if (changed)
        {
            eulerRotation = MathUtils::DegToRad(eulerRotation);
            Quaternion newRotation = Quaternion::CreateFromYawPitchRoll(eulerRotation.y, eulerRotation.x, eulerRotation.z);
            boneInfo.BoneLocalTransform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(newRotation) * Matrix::CreateTranslation(position);
            _skinnedMesh.RecalcBoneTransforms(_debugBoneIndex);
        }

		DrawDebugMatrix("Bone local matrix", boneInfo.BoneLocalTransform);
		DrawDebugMatrix("Global matrix", boneInfo.GlobalTransformation);
		DrawDebugMatrix("Global inverse matrix", boneInfo.GlobalTransformationInverse);
		DrawDebugMatrix("Bone offset matrix", boneInfo.OffsetMatrix);
		DrawDebugMatrix("Final bone matrix", boneInfo.FinalTransformation);

        EDITOR->GetSceneView()->SetTransformGizmoOverride(boneInfo.GlobalTransformation * GetTransform()->GetWorldMatrix(), [&](const Matrix& worldMatrix)
            {
                Matrix modelMatrix = worldMatrix * GetTransform()->GetWorldMatrix().Invert();

                Matrix parentGlobalInverse = Matrix::Identity;
                if (boneInfo.ParentIndex >= 0)
                    parentGlobalInverse = boneInfos[boneInfo.ParentIndex].GlobalTransformationInverse;
                boneInfo.BoneLocalTransform = modelMatrix * parentGlobalInverse;
                _skinnedMesh.RecalcBoneTransforms(_debugBoneIndex);
            });
	}
	ImGui::EndChild();
	ImGui::End();
}

void ModelAnimator::DrawDebugMatrix(const char* label, const Matrix& matrix)
{
	if (!ImGui::TreeNode(label))
		return;

	if (ImGui::BeginTable("##Matrix", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
	{
		const float* values = &matrix._11;
		for (int row = 0; row < 4; ++row)
		{
			ImGui::TableNextRow();
			for (int column = 0; column < 4; ++column)
			{
				ImGui::TableSetColumnIndex(column);
				ImGui::Text("%.5f", values[row * 4 + column]);
			}
		}
		ImGui::EndTable();
	}
	ImGui::TreePop();
}
