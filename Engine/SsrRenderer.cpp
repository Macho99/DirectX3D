#include "pch.h"
#include "SsrRenderer.h"
#include "OnGUIUtils.h"

SsrRenderer::SsrRenderer()
    :Super(StaticType)
{
}

SsrRenderer::~SsrRenderer()
{
}

void SsrRenderer::InnerRender(RenderTech renderTech)
{
    Super::InnerRender(renderTech);
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
	material->GetShader()->DrawIndexed(renderTech, _pass, mesh->GetIndexBuffer()->GetCount());
}

bool SsrRenderer::OnGUI()
{
	bool changed = false;
	changed |= Super::OnGUI();
	ImGui::Separator();
	changed |= OnGUIUtils::DrawResourceRef("Mesh", _mesh);

    return changed;
}

void SsrRenderer::SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris)
{
	if (GetGameObject()->IsActiveInHierarchy() == false)
		return;

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

        tri.v0 = Vec3::Max(tri.v0, explicitBounds.bmin);
        tri.v0 = Vec3::Min(tri.v0, explicitBounds.bmax);
        tri.v1 = Vec3::Max(tri.v1, explicitBounds.bmin);
        tri.v1 = Vec3::Min(tri.v1, explicitBounds.bmax);
        tri.v2 = Vec3::Max(tri.v2, explicitBounds.bmin);
        tri.v2 = Vec3::Min(tri.v2, explicitBounds.bmax);

		tri.walkable = false;

		if (explicitBounds.IsInside(tri))
			tris.push_back(tri);
	}
}
