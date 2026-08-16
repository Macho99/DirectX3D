#include "pch.h"
#include "ModelRenderer.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "Camera.h"
#include "OnGUIUtils.h"
#include "FoliageController.h"

ModelRenderer::ModelRenderer()
	: Super(StaticType)
{
	_pass = 1;
}

ModelRenderer::~ModelRenderer()
{

}

bool ModelRenderer::SetModel(ResourceRef<Model> model)
{
	Model* modelPtr = model.Resolve();
	if (modelPtr == nullptr || modelPtr->HasValidRenderResources() == false)
		return false;

	const AssetId oldMeshId = GetMeshId();
	_model = model; 
	_boundsInitialized = false;
	OnMeshChange(oldMeshId, GetMeshId());

	_initialized = false;
	bool isFoliage = false;

	auto& materials = modelPtr->GetMaterials();
	for (auto& materialRef : materials)
	{
		Material* material = materialRef.Resolve();
		if (material == nullptr)
			continue;

		if (material->GetShaderRef() == RESOURCES->GetFoliageShader())
			isFoliage = true;
	}
	for (auto& materialRef : materials)
	{
		Material* material = materialRef.Resolve();
		if (material == nullptr)
			continue;

		if (material->GetShader() == nullptr)
		{
			if (isFoliage)
			{
				material->SetShader(RESOURCES->GetFoliageShader());
			}
			else
			{
				material->SetShader(RESOURCES->GetDefaultShader());
			}
		}
	}

	SetMaterial(materials[0]);

	if (isFoliage)
	{
		FoliageSetup();
	}

	return true;
}

bool ModelRenderer::IsInFrustum(const Vec4 frustumPlanes[6])
{
	if (_boundsInitialized == false)
		UpdateLocalBounds();

	if (_hasLocalBounds == false || HasInstancingData())
		return true;

	const BoundingBox& worldBounds = _worldBounds;

	for (uint32 i = 0; i < 6; ++i)
	{
		const Vec4& plane = frustumPlanes[i];
		const float centerDistance =
			plane.x * worldBounds.Center.x
			+ plane.y * worldBounds.Center.y
			+ plane.z * worldBounds.Center.z
			+ plane.w;
		const float projectedRadius =
			fabsf(plane.x) * worldBounds.Extents.x
			+ fabsf(plane.y) * worldBounds.Extents.y
			+ fabsf(plane.z) * worldBounds.Extents.z;

		if (centerDistance + projectedRadius < 0.0f)
			return false;
	}

	return true;
}

void ModelRenderer::RenderInstancing(InstancingBuffer& buffer, RenderTech renderTech)
{
    Model* model = _model.Resolve();
	if (model == nullptr)
		return;
    ModelMeshResource* mesh = model->GetMesh();
    if (mesh == nullptr)
        return;

    if (!TryInitialize())
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

	// Bones
	//BoneDesc boneDesc;
	//
	//const uint32 boneCount = mesh->GetBoneCount();
	//for (uint32 i = 0; i < boneCount; i++)
	//{
	//	shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(i);
	//	boneDesc.transforms[i] = bone->globalMatrix;
	//}
	//shader->PushBoneData(boneDesc);

	const auto& meshes = mesh->GetMeshes();
	for (auto& mesh : meshes)
	{
        Material* material = mesh->material.Resolve();
		if (material)
			material->Update();

		// BoneIndex
		//shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		// IA
		mesh->vertexBuffer->PushData();
		mesh->indexBuffer->PushData();

		buffer.PushData();

		material->GetShader()->DrawIndexedInstanced(renderTech, _pass, mesh->indexBuffer->GetCount(), buffer.GetCount());
	}
}

bool ModelRenderer::OnGUI()
{
	bool changed = false;
	changed |= Super::OnGUI();
	ImGui::Separator();
	ResourceRef<Model> model = _model;
	if (OnGUIUtils::DrawResourceRef("Model", model))
	{
		changed |= SetModel(model);
	}
	return changed;
}

void ModelRenderer::OnMenu()
{
	if (ImGui::MenuItem("Set Foliage Setting"))
	{
        FoliageSetup();
	}
}

void ModelRenderer::OnEnable()
{
	_boundsInitialized = false;
}

bool ModelRenderer::TryInitialize()
{
    if (_initialized)
        return true;

	Model* modelPtr = _model.Resolve();
	if(modelPtr == nullptr)
        return false;
		
    _initialized = true;
    return true;
}

void ModelRenderer::SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris)
{
	if (GetGameObject()->IsActiveInHierarchy() == false)
		return;

    Model* model = _model.Resolve();
    if (model == nullptr)
        return;

    ModelMeshResource* mesh = model->GetMesh();
    if (mesh == nullptr)
        return;

    Transform* transform = GetGameObject()->GetTransform();
    Matrix worldMat = transform->GetWorldMatrix();

    auto& meshes = mesh->GetMeshes();
    for (shared_ptr<ModelMesh>& mesh : meshes)
    {
		Material* material = mesh->material.Resolve();
		if (material == nullptr || material->IsIncludeInNavMesh() == false)
			return;

        Matrix finalMat = worldMat;
		if (mesh->bone != nullptr)
		{
			const auto& bone = mesh->bone;
			Matrix boneMat = bone->globalMatrix;
			finalMat = boneMat * worldMat;
		}

        const auto& vertices = mesh->geometry->GetVertices();
        const auto& indices = mesh->geometry->GetIndices();
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            InputTri tri;
            tri.v0 = vertices[indices[i + 0]].position;
            tri.v0 = Vec3::Transform(tri.v0, finalMat);
            tri.v1 = vertices[indices[i + 1]].position;
            tri.v1 = Vec3::Transform(tri.v1, finalMat);
            tri.v2 = vertices[indices[i + 2]].position;
            tri.v2 = Vec3::Transform(tri.v2, finalMat);

			if(explicitBounds.IsInside(tri))
				tris.push_back(tri);
        }
    }
}

void ModelRenderer::FoliageSetup()
{
	_pass = 0;

	if (GetGameObject()->GetFixedComponent<FoliageController>() == nullptr)
		GetGameObject()->AddComponent<FoliageController>();
}

void ModelRenderer::UpdateLocalBounds()
{
	_hasLocalBounds = false;

	Model* model = _model.Resolve();
	ModelMeshResource* meshResource = model != nullptr ? model->GetMesh() : nullptr;
	if (meshResource == nullptr)
		return;

	Vec3 minPosition(FLT_MAX, FLT_MAX, FLT_MAX);
	Vec3 maxPosition(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (const shared_ptr<ModelMesh>& mesh : meshResource->GetMeshes())
	{
		if (mesh == nullptr || mesh->geometry == nullptr)
			continue;

		for (const ModelVertexType& vertex : mesh->geometry->GetVertices())
		{
			minPosition = Vec3::Min(minPosition, vertex.position);
			maxPosition = Vec3::Max(maxPosition, vertex.position);
			_hasLocalBounds = true;
		}
	}

	if (_hasLocalBounds)
	{
		_localBounds.Center = (minPosition + maxPosition) * 0.5f;
		_localBounds.Extents = (maxPosition - minPosition) * 0.5f;
	}


	_localBounds.Transform(_worldBounds, GetTransform()->GetWorldMatrix());
	_boundsInitialized = true;
}
