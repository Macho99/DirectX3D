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

	if (_mesh == nullptr)
		return false;

	// Light
	//_material->Update();

	_mesh->GetVertexBuffer()->PushData();
	_mesh->GetIndexBuffer()->PushData();
    _material->GetShader()->DrawIndexed(renderTech, _pass, _mesh->GetIndexBuffer()->GetCount());
	return true;
}

void MeshRenderer::RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech)
{
	if (Super::Render(renderTech) == false)
	{
		return;
	}

	if (_mesh == nullptr)
		return;

	// Light
	//_material->Update();

	_mesh->GetVertexBuffer()->PushData();
	_mesh->GetIndexBuffer()->PushData();

	buffer->PushData();

	_material->GetShader()->DrawIndexedInstanced(renderTech, _pass, _mesh->GetIndexBuffer()->GetCount(),
		buffer->GetCount());
}

InstanceID MeshRenderer::GetInstanceID()
{
	return make_pair((uint64)_mesh.get(), (uint64)_material.get());
}
