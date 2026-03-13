#include "pch.h"
#include "ModelRenderer.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "Camera.h"
#include "OnGUIUtils.h"

ModelRenderer::ModelRenderer()
	: Super(StaticType)
{

}

ModelRenderer::~ModelRenderer()
{

}

void ModelRenderer::SetShader(ResourceRef<Shader> shader)
{
    _shader = shader;
}

void ModelRenderer::SetModel(ResourceRef<Model> model)
{
	_model = model;
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

		// BoneIndex
		shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

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

bool ModelRenderer::OnGUI()
{
	bool changed = false;
	changed |= Super::OnGUI();
	ImGui::Separator();
    changed |= OnGUIUtils::DrawResourceRef("Model", _model);
    changed |= OnGUIUtils::DrawResourceRef("Shader", _shader);
	return changed;
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
		_material = material;
    }

    _initialized = true;
    return true;
}
