#include "pch.h"
#include "ScrollView.h"
#include "OnGUIUtils.h"

ScrollView::ScrollView()
    : Super(StaticType)
{
    _mesh = RESOURCES->GetQuadMesh();
    _maskMode = UIMaskMode::VisibleMask;
    _material = RESOURCES->AllocateUIDefaultMaterial();
}

void ScrollView::Awake()
{
    GameObjectRef contentRef = CUR_SCENE->Add("Content", GetTransform());
    RectTransform* contentRect = static_cast<RectTransform*>(contentRef.Resolve()->GetTransform());
    _contentRectTransformRef = RectTransformRef(contentRect);
}

void ScrollView::OnBeginDrag()
{
}

void ScrollView::OnDrag(DragEvent event)
{
    RectTransform* contentRectTransform = _contentRectTransformRef.Resolve();
    if (_horizontalScrollEnabled == false)
        event.delta.x = 0.f;
    if (_verticalScrollEnabled == false)
        event.delta.y = 0.f;

    contentRectTransform->MoveOffsets(Vec2(event.delta.x, -event.delta.y));
}

void ScrollView::OnEndDrag()
{
}

bool ScrollView::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();

    ImGui::Separator();
    changed |= OnGUIUtils::DrawComponentRef("Content RectTransform", _contentRectTransformRef);

    if (OnGUIUtils::DrawResourceRef("Background Texture", _backgroundTexture))
    {
        SetBackgroundTexture(_backgroundTexture);
        changed = true;
    }

    changed |= OnGUIUtils::DrawBool("Horizontal Scroll Enabled", &_horizontalScrollEnabled);
    changed |= OnGUIUtils::DrawBool("Vertical Scroll Enabled", &_verticalScrollEnabled);

    return changed;
}

void ScrollView::SetBackgroundTexture(ResourceRef<Texture> texture)
{
    _backgroundTexture = texture;
    _material.Resolve()->SetDiffuseMap(texture);
}