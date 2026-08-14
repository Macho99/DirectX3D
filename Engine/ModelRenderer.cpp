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

void ModelRenderer::SetShader(ResourceRef<Shader> shader)
{
    _shader = shader;
	_initialized = false;
	Super::SetMaterial(ResourceRef<Material>());
}

void ModelRenderer::SetModel(ResourceRef<Model> model)
{
	const AssetId oldMeshId = GetMeshId();
	Super::SetMaterial(ResourceRef<Material>());
	_model = model;
	OnMeshChange(oldMeshId, GetMeshId());
	_initialized = false;
	if (_shader.Resolve() == nullptr)
	{
        bool isFoliage = false;
        Model* modelPtr = _model.Resolve();
		if (modelPtr != nullptr)
		{
			const auto& materials = modelPtr->GetMaterials();
			if (materials.size() > 0)
			{
				ResourceRef<Shader> shaderRef = materials[0].Resolve()->GetShaderRef();
				ResourceRef<Shader> foliageShaderRef = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\Foliage.fx");

				if (shaderRef == foliageShaderRef)
				{
					FoliageSetup();
                    isFoliage = true;
				}
			}
		}

		if (isFoliage == false)
		{
			SetShader(RESOURCES->GetDefaultShader());
		}
	}

}

void ModelRenderer::RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech)
{
    Model* model = _model.Resolve();
	if (model == nullptr)
		return;
    ModelMeshResource* mesh = model->GetMesh();
    if (mesh == nullptr)
        return;
    Shader* shader = _shader.Resolve();
    if (shader == nullptr)
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

		buffer->PushData();

		shader->DrawIndexedInstanced(renderTech, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelRenderer::GetInstanceID()
{
	return make_pair((uint64)_model.GetAssetId().GetLeftId(), (uint64)_shader.GetAssetId().GetLeftId());
}

void ModelRenderer::SetMaterial(ResourceRef<Material> material)
{
	ASSERT(false, "ModelRenderer::SetMaterial is not supported. Use SetModel instead.");
}

void ModelRenderer::LateUpdate()
{
	if (HasInstancingData())
	{
        const Matrix worldMat = GetTransform()->GetWorldMatrix();
        if (worldMat != _lastWorldMatrix || _instDatas.size() != _originInstDatas.size())
        {
            _instDatas.resize(_originInstDatas.size());
            for (size_t i = 0; i < _originInstDatas.size(); i++)
            {
                _instDatas[i] = _originInstDatas[i] * worldMat;
            }
        }
        _lastWorldMatrix = worldMat;
	}
}

bool ModelRenderer::OnGUI()
{
	bool changed = false;
	changed |= Super::OnGUI();
	ImGui::Separator();
	if (OnGUIUtils::DrawResourceRef("Model", _model))
	{
        SetModel(_model);
        changed = true;
	}
    if (OnGUIUtils::DrawResourceRef("Shader", _shader))
    {
        SetShader(_shader);
        changed = true;
    }
	return changed;
}

void ModelRenderer::OnMenu()
{
    if (ImGui::MenuItem("Set Default Shader"))
    {
        SetShader(RESOURCES->GetDefaultShader());
    }

	if (ImGui::MenuItem("Set Foliage Setting"))
	{
        FoliageSetup();
	}
}

bool ModelRenderer::TryInitialize()
{
    if (_initialized)
        return true;

	Model* modelPtr = _model.Resolve();
	if(modelPtr == nullptr)
        return false;

    Shader* shaderPtr = _shader.Resolve();
    if (shaderPtr == nullptr)
        return false;

	auto& materials = modelPtr->GetMaterials();
	for (auto& material : materials)
	{
		if (material.Resolve() == nullptr)
			continue;
		material.Resolve()->SetShader(_shader);
		Super::SetMaterial(material);
    }

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

void ModelRenderer::AddInstancingData(const Matrix& mat)
{
    _originInstDatas.push_back(mat);
}

void ModelRenderer::FoliageSetup()
{
	_pass = 0;
	SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\Foliage.fx"));

	if (GetGameObject()->GetFixedComponent<FoliageController>() == nullptr)
		GetGameObject()->AddComponent<FoliageController>();
}
