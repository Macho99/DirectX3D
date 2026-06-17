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

Vec2 UIRenderer::GetMousePosition() const
{
    const POINT& mousePos = INPUT->GetMousePos();
    const GameDesc& gameDesc = GAME->GetGameDesc();

    return Vec2(
        static_cast<float>(mousePos.x) - gameDesc.sceneWidth * 0.5f,
        gameDesc.sceneHeight * 0.5f - static_cast<float>(mousePos.y)
    );
}

Vec2 UIRenderer::GetLocalMousePosition()
{
    Transform* transform = GetTransform();
    if (transform == nullptr)
        return Vec2(0.0f, 0.0f);

    const Vec2 mousePos = GetMousePosition();
    Vec3 mouseWorld = Vec3(mousePos.x, mousePos.y, 0.0f);
    Vec3 mouseLocal = Vec3::Transform(mouseWorld, transform->GetWorldMatrix().Invert());
    return Vec2(mouseLocal.x, mouseLocal.y);
}

bool UIRenderer::ContainsMouseSelf()
{
    if (GAME->GetGameDesc().isEditor && !INPUT->IsMouseInScene())
        return false;

    const Vec2 mouseLocal = GetLocalMousePosition();
    return -0.5f <= mouseLocal.x && mouseLocal.x <= 0.5f
        && -0.5f <= mouseLocal.y && mouseLocal.y <= 0.5f;
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
