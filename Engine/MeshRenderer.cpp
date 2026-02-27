#include "pch.h"
#include "MeshRenderer.h"
#include "Game.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Light.h"

MeshRenderer::MeshRenderer() : Super(ComponentType::MeshRenderer)
{

}

MeshRenderer::~MeshRenderer()
{

}

bool MeshRenderer::Render(RenderTech renderTech)
{
	if (Super::Render(renderTech) == false)
	{
		return false;
	}

	Mesh* mesh = _mesh.Resolve();
	if (mesh == nullptr)
		return false;
    Material* material = _material.Resolve();
    if (material == nullptr)
        return false;

	// Light
	//_material->Update();

	mesh->GetVertexBuffer()->PushData();
	mesh->GetIndexBuffer()->PushData();
	material->GetShader()->DrawIndexed(renderTech, _pass, mesh->GetIndexBuffer()->GetCount());
	return true;
}

void MeshRenderer::RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech)
{
	if (Super::Render(renderTech) == false)
	{
		return;
	}
	Mesh* mesh = _mesh.Resolve();
	if (mesh == nullptr)
		return;
	Material* material = _material.Resolve();
	if (material == nullptr)
		return;

	// Light
	//_material->Update();

	mesh->GetVertexBuffer()->PushData();
	mesh->GetIndexBuffer()->PushData();

	buffer->PushData();

	material->GetShader()->DrawIndexedInstanced(renderTech, _pass, mesh->GetIndexBuffer()->GetCount(),
		buffer->GetCount());
}

InstanceID MeshRenderer::GetInstanceID()
{
	return make_pair((uint64)_mesh.GetAssetId().GetLeftId(), (uint64)_material.GetAssetId().GetLeftId());
}
