#include "pch.h"
#include "UIRenderer.h"
#include "GameObject.h"
#include "Graphics.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshRenderer.h"
#include "OnGUIUtils.h"
#include "ResourceManager.h"

UIRenderer::UIRenderer(ComponentType componentType)
    : Super(componentType)
{
}

bool UIRenderer::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();

    changed |= OnGUIUtils::DrawResourceRef("Mesh", _mesh);

    if (OnGUIUtils::DrawEnumCombo("Mask Mode", _maskMode, UIMaskModeNames, (int)UIMaskMode::Max))
    {
        SetMaskMode(_maskMode);
        changed = true;
    }

    return changed;
}

void UIRenderer::SetMaskMode(UIMaskMode mode)
{
    _maskMode = mode;
}

void UIRenderer::InnerRender(RenderTech renderTech)
{
    Super::InnerRender(renderTech);

    Mesh* mesh = _mesh.Resolve();
    if (mesh == nullptr)
    {
        return;
    }

    Material* material = _material.Resolve();
    if (material == nullptr)
    {
        return;
    }

    if (mesh->GetVertexBuffer() == nullptr || mesh->GetIndexBuffer() == nullptr)
    {
        return;
    }
    mesh->GetVertexBuffer()->PushData();
    mesh->GetIndexBuffer()->PushData();

    Shader* shader = material->GetShader();
    int technique = shader->GetTechNum(renderTech);
    shader->BeginDraw(technique, _pass);
    GRAPHICS->ApplyUIMaskState(_maskMode);
    DC->DrawIndexed(mesh->GetIndexBuffer()->GetCount(), 0, 0);
    //material->GetShader()->DrawIndexed(renderTech, _pass, mesh->GetIndexBuffer()->GetCount());
    GRAPHICS->ResetUIMaskState();
    shader->EndDraw(technique, _pass);
}
