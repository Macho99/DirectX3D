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
