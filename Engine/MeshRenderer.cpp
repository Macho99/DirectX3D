#include "pch.h"
#include "MeshRenderer.h"
#include "Game.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Light.h"
#include "OnGUIUtils.h"

MeshRenderer::MeshRenderer() : Super(StaticType)
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

bool MeshRenderer::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
	ImGui::Separator();
    changed |= OnGUIUtils::DrawResourceRef("Mesh", _mesh);
    return changed;
}

void MeshRenderer::SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris)
{
	Mesh* mesh = _mesh.Resolve();
	if (mesh == nullptr)
		return;

	Material* material = GetMaterial().Resolve();
	if (material == nullptr || material->IsIncludeInNavMesh() == false)
		return;

	shared_ptr<Geometry<VertexTextureNormalTangentData>> geometry = mesh->GetGeometry();
	const vector<VertexTextureNormalTangentData>& vertices = geometry->GetVertices();
    const vector<uint32>& indices = geometry->GetIndices();

    ASSERT(indices.size() % 3 == 0);
    Transform* transform = GetGameObject()->GetTransform();
	Matrix worldMat = transform->GetWorldMatrix();

    for (int i = 0; i < indices.size(); i += 3)
    {
        InputTri tri;
        tri.v0 = vertices[indices[i + 0]].position;
        tri.v0 = Vec3::Transform(tri.v0, worldMat);

        tri.v1 = vertices[indices[i + 1]].position;
        tri.v1 = Vec3::Transform(tri.v1, worldMat);

        tri.v2 = vertices[indices[i + 2]].position;
        tri.v2 = Vec3::Transform(tri.v2, worldMat);

		if (explicitBounds.IsInside(tri))
			tris.push_back(tri);
    }
}
