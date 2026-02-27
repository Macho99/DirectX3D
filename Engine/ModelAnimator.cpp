#include "pch.h"
#include "ModelAnimator.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "ModelAnimation.h"
#include "Camera.h"
#include "Light.h"

ModelAnimator::ModelAnimator(ResourceRef<Shader> shader)
	: Super(ComponentType::Animator), _shader(shader)
{
	// TEST
	_tweenDesc.next.animIndex = rand() % 3;
	_tweenDesc.tweenSumTime += rand();
	_tweenDesc.next.speed = (rand() % 50) / 10.0f + 1.0f;
}

ModelAnimator::~ModelAnimator()
{
}

void ModelAnimator::Update()
{
	UpdateTweenData();
}

void ModelAnimator::SetModel(ResourceRef<Model> model)
{
	_model = model;

	auto& materials = _model.Resolve()->GetMaterials();
	for (auto& material : materials)
	{
		material.Resolve()->SetShader(_shader);
		_material = material;
		break;
	}
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

	if (Super::Render(renderTech) == false)
		return;

	//// GlobalData
	//_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	//
	//// Light
	//auto lightObj = SCENE->GetCurrentScene()->GetLight();
	//if (lightObj)
	//	_shader->PushLightData(lightObj->GetLight()->GetLightDesc());

	if (_texture == nullptr)
		CreateTexture();

	// SRV를 통해 정보 전달
	_shader.Resolve()->GetSRV("TransformMap")->SetResource(_srv.Get());

	// Bone
	BoneDesc boneDesc;

	const uint32 boneCount = mesh->GetBoneCount();
	for (uint32 i = 0; i < boneCount; i++)
	{
		shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(i);
		boneDesc.transforms[i] = bone->transform;
	}
	shader->PushBoneData(boneDesc);

	const auto& meshes = mesh->GetMeshes();
	for (auto& mesh : meshes)
	{
        Material* material = mesh->material.Resolve();
		if (material)
			material->Update();

		shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		mesh->vertexBuffer->PushData();
		mesh->indexBuffer->PushData();
		buffer->PushData();
		shader->DrawIndexedInstanced(renderTech, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelAnimator::GetInstanceID()
{
	return make_pair((uint64)_model.GetAssetId().GetLeftId(), (uint64)_shader.GetAssetId().GetLeftId());
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
			shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(b);

			Matrix matAnim;

			shared_ptr<ModelKeyframe> frame = animation->GetKeyframe(bone->name);
			if (frame != nullptr)
			{
				ModelKeyframeData& data = frame->transforms[f];
				Matrix S, R, T;
				S = Matrix::CreateScale(data.scale);
				R = Matrix::CreateFromQuaternion(data.rotation);
				T = Matrix::CreateTranslation(data.translation);

				matAnim = S * R * T;
			}
			else
			{
				matAnim = Matrix::Identity;
			}

			Matrix toRootMat = bone->transform;
			Matrix invGlobal = toRootMat.Invert();

			int32 parentIndex = bone->parentIndex;

			Matrix matParent = Matrix::Identity;
			if (parentIndex >= 0)
			{
				matParent = tempAnimBoneTransforms[parentIndex];
			}

			tempAnimBoneTransforms[b] = matAnim * matParent;
			_animTransforms[index].transforms[f][b] = invGlobal * tempAnimBoneTransforms[b];
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
			float timePerFrame = 1 / (currentAnim->GetFrameRate() * desc.cur.speed);
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
			ModelAnimation* nextAnim = model->GetAnimationByIndex(desc.next.animIndex);
			desc.next.sumTime += DT;

			float timePerFrame = 1.f / (nextAnim->GetFrameCount() * desc.next.speed);

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