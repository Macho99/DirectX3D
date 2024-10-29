#include "pch.h"
#include "ModelAnimator.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "ModelAnimation.h"

ModelAnimator::ModelAnimator(shared_ptr<Shader> shader)
	: Super(ComponentType::Animator), _shader(shader)
{
}

ModelAnimator::~ModelAnimator()
{
}

//void ModelAnimator::Update()
//{
//	if (_model == nullptr)
//		return;
//
//	if (_texture == nullptr)
//		CreateTexture();
//
//	_keyframeDesc.sumTime += DT;
//
//	shared_ptr<ModelAnimation> current = _model->GetAnimationByIndex(_keyframeDesc.animIndex);
//	if (current)
//	{
//		float timePerFrame = 1 / (current->frameRate * _keyframeDesc.speed);
//		if (_keyframeDesc.sumTime >= timePerFrame)
//		{
//			_keyframeDesc.sumTime = 0.f;
//			_keyframeDesc.curFrame = (_keyframeDesc.curFrame + 1) % current->frameCount;
//			_keyframeDesc.nexFrame = (_keyframeDesc.curFrame + 1) % current->frameCount;
//		}
//
//		_keyframeDesc.ratio = (_keyframeDesc.sumTime / timePerFrame);
//	}
//
//	// Anim Update
//	ImGui::InputInt("AnimIndex", &_keyframeDesc.animIndex);
//	_keyframeDesc.animIndex %= _model->GetAnimationCount();
//	ImGui::InputInt("CurFrame", (int*)&_keyframeDesc.curFrame);
//	_keyframeDesc.curFrame %= _model->GetAnimationByIndex(_keyframeDesc.animIndex)->frameCount;
//	ImGui::InputFloat("Speed", &_keyframeDesc.speed, 0.5f, 4.f);
//
//	// 애니메이션 현재 프레임 정보
//	RENDER->PushKeyframeData(_keyframeDesc);
//
//	// SRV를 통해 정보 전달
//	_shader->GetSRV("TransformMap")->SetResource(_srv.Get());
//
//	// Bone
//	BoneDesc boneDesc;
//
//	const uint32 boneCount = _model->GetBoneCount();
//	for (uint32 i = 0; i < boneCount; i++)
//	{
//		shared_ptr<ModelBone> bone = _model->GetBoneByIndex(i);
//		boneDesc.transforms[i] = bone->transform;
//	}
//	RENDER->PushBoneData(boneDesc);
//
//	Matrix world = GetTransform()->GetWorldMatrix();
//	RENDER->PushTransformData(TransformDesc{ world });
//
//	const auto& meshes = _model->GetMeshes();
//	for (auto& mesh : meshes)
//	{
//		if (mesh->material)
//			mesh->material->Update();
//
//		_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);
//
//		uint32 stride = mesh->vertexBuffer->GetStride();
//		uint32 offset = mesh->vertexBuffer->GetOffset();
//
//		DC->IASetVertexBuffers(0, 1, mesh->vertexBuffer->GetComPtr().GetAddressOf(), &stride, &offset);
//		DC->IASetIndexBuffer(mesh->indexBuffer->GetComPtr().Get(), DXGI_FORMAT_R32_UINT, 0);
//
//		_shader->DrawIndexed(0, _pass, mesh->indexBuffer->GetCount(), 0, 0);
//	}
//}

void ModelAnimator::Update()
{
	if (_model == nullptr)
		return;

	if (_texture == nullptr)
		CreateTexture();

	TweenDesc& desc = _tweenDesc;

	desc.cur.sumTime += DT;
	// 현재 애니메이션
	{
		shared_ptr<ModelAnimation> currentAnim = _model->GetAnimationByIndex(desc.cur.animIndex);
		if (currentAnim)
		{
			float timePerFrame = 1 / (currentAnim->frameRate * desc.cur.speed);
			if (desc.cur.sumTime >= timePerFrame)
			{
				desc.cur.sumTime = 0;
				desc.cur.curFrame = (desc.cur.curFrame + 1) % currentAnim->frameCount;
				desc.cur.nextFrame = (desc.cur.curFrame + 1) % currentAnim->frameCount;
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
			shared_ptr<ModelAnimation> nextAnim = _model->GetAnimationByIndex(desc.next.animIndex);
			desc.next.sumTime += DT;

			float timePerFrame = 1.f / (nextAnim->frameRate * desc.next.speed);

			if (desc.next.ratio >= 1.f)
			{
				desc.next.sumTime = 0;

				desc.next.curFrame = (desc.next.curFrame + 1) % nextAnim->frameCount;
				desc.next.nextFrame = (desc.next.curFrame + 1) % nextAnim->frameCount;
			}

			desc.next.ratio = desc.next.sumTime / timePerFrame;
		}
	}

	// Anim Update
	ImGui::InputInt("AnimIndex", &desc.cur.animIndex);
	desc.cur.animIndex %= _model->GetAnimationCount();

	static int32 nextAnimIndex = 0;
	if (ImGui::InputInt("NextAnimIndex", &nextAnimIndex))
	{
		nextAnimIndex %= _model->GetAnimationCount();
		desc.ClearNextAnim(); // 기존꺼 밀어주기
		desc.next.animIndex = nextAnimIndex;
	}

	ImGui::InputInt("CurFrame", (int*)&_keyframeDesc.curFrame);
	_keyframeDesc.curFrame %= _model->GetAnimationByIndex(_keyframeDesc.animIndex)->frameCount;
	ImGui::InputFloat("Speed", &_keyframeDesc.speed, 0.5f, 4.f);

	RENDER->PushTweenData(desc);

	// SRV를 통해 정보 전달
	_shader->GetSRV("TransformMap")->SetResource(_srv.Get());

	// Bone
	BoneDesc boneDesc;

	const uint32 boneCount = _model->GetBoneCount();
	for (uint32 i = 0; i < boneCount; i++)
	{
		shared_ptr<ModelBone> bone = _model->GetBoneByIndex(i);
		boneDesc.transforms[i] = bone->transform;
	}
	RENDER->PushBoneData(boneDesc);

	Matrix world = GetTransform()->GetWorldMatrix();
	RENDER->PushTransformData(TransformDesc{ world });

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
			mesh->material->Update();

		_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		uint32 stride = mesh->vertexBuffer->GetStride();
		uint32 offset = mesh->vertexBuffer->GetOffset();

		DC->IASetVertexBuffers(0, 1, mesh->vertexBuffer->GetComPtr().GetAddressOf(), &stride, &offset);
		DC->IASetIndexBuffer(mesh->indexBuffer->GetComPtr().Get(), DXGI_FORMAT_R32_UINT, 0);

		_shader->DrawIndexed(0, _pass, mesh->indexBuffer->GetCount(), 0, 0);
	}
}

void ModelAnimator::SetModel(shared_ptr<Model> model)
{
	_model = model;

	const auto& materials = _model->GetMaterials();
	for (auto& material : materials)
	{
		material->SetShader(_shader);
	}
}

void ModelAnimator::CreateTexture()
{
	if (_model->GetAnimationCount() == 0)
		return;

	_animTransforms.resize(_model->GetAnimationCount());

	for (uint32 i = 0; i < _model->GetAnimationCount(); i++)
	{
		CreateAnimationTransform(i);
	}

	// Creature Texture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = MAX_MODEL_TRANSFORMS * 4;
		desc.Height = MAX_MODEL_KEYFRAMES;
		desc.ArraySize = _model->GetAnimationCount();
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // 16바이트
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;

		const uint32 dataSize = MAX_MODEL_TRANSFORMS * sizeof(Matrix);
		const uint32 pageSize = dataSize * MAX_MODEL_KEYFRAMES;
		void* mallocPtr = ::malloc(pageSize * _model->GetAnimationCount());

		// 파편화된 데이터를 조립한다.
		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
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
		vector<D3D11_SUBRESOURCE_DATA> subResources(_model->GetAnimationCount());

		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			void* ptr = (BYTE*)mallocPtr + c * pageSize;
			subResources[c].pSysMem = ptr;
			subResources[c].SysMemPitch = dataSize;
			subResources[c].SysMemSlicePitch = pageSize;
		}

		HRESULT hr = DEVICE->CreateTexture2D(&desc, subResources.data(), _texture.GetAddressOf());
		CHECK(hr);

		::free(mallocPtr);
	}

	// Create SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipLevels = 1;
		desc.Texture2DArray.ArraySize = _model->GetAnimationCount();

		HRESULT hr = DEVICE->CreateShaderResourceView(_texture.Get(), &desc, _srv.GetAddressOf());
		CHECK(hr);
	}
}

void ModelAnimator::CreateAnimationTransform(uint32 index)
{
	vector<Matrix> tempAnimBoneTransforms(MAX_MODEL_TRANSFORMS, Matrix::Identity);
	shared_ptr<ModelAnimation> animation = _model->GetAnimationByIndex(index);

	for (uint32 f = 0; f < animation->frameCount; f++)
	{
		for (uint32 b = 0; b < _model->GetBoneCount(); b++)
		{
			shared_ptr<ModelBone> bone = _model->GetBoneByIndex(b);

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
