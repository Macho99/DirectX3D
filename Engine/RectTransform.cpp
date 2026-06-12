#include "pch.h"
#include "RectTransform.h"
#include "OnGUIUtils.h"

RectTransform::RectTransform()
    : Super(StaticType)
{
}

RectTransform::~RectTransform()
{
}

void RectTransform::Awake()
{
    Super::Awake();
    ApplyToTransform();
}

bool RectTransform::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();

    Vec2 anchorMin = _anchorMin;
    if (OnGUIUtils::DrawVec2("Anchor Min", &anchorMin, 0.01f))
    {
        SetAnchorMin(anchorMin);
        changed = true;
    }

    Vec2 anchorMax = _anchorMax;
    if (OnGUIUtils::DrawVec2("Anchor Max", &anchorMax, 0.01f))
    {
        SetAnchorMax(anchorMax);
        changed = true;
    }

    Vec2 offsetMin = _offsetMin;
    if (OnGUIUtils::DrawVec2("Offset Min", &offsetMin, 1.f))
    {
        SetOffsetMin(offsetMin);
        changed = true;
    }

    Vec2 offsetMax = _offsetMax;
    if (OnGUIUtils::DrawVec2("Offset Max", &offsetMax, 1.f))
    {
        SetOffsetMax(offsetMax);
        changed = true;
    }

    Vec2 pivot = _pivot;
    if (OnGUIUtils::DrawVec2("Pivot", &pivot, 0.01f))
    {
        SetPivot(pivot);
        changed = true;
    }

    return changed;
}

void RectTransform::SetAnchorMin(const Vec2& anchorMin)
{
    _anchorMin = anchorMin;
    ApplyToTransform();
}

void RectTransform::SetAnchorMax(const Vec2& anchorMax)
{
    _anchorMax = anchorMax;
    ApplyToTransform();
}

void RectTransform::SetAnchors(const Vec2& anchorMin, const Vec2& anchorMax)
{
    _anchorMin = anchorMin;
    _anchorMax = anchorMax;
    ApplyToTransform();
}

void RectTransform::SetOffsetMin(const Vec2& offsetMin)
{
    _offsetMin = offsetMin;
    ApplyToTransform();
}

void RectTransform::SetOffsetMax(const Vec2& offsetMax)
{
    _offsetMax = offsetMax;
    ApplyToTransform();
}

void RectTransform::SetOffsets(const Vec2& offsetMin, const Vec2& offsetMax)
{
    _offsetMin = offsetMin;
    _offsetMax = offsetMax;
    ApplyToTransform();
}

void RectTransform::SetPivot(const Vec2& pivot)
{
    _pivot = pivot;
    ApplyToTransform();
}

RectTransformRect RectTransform::GetRect() const
{
    RectTransformRect parentRect = GetParentRect();
    Vec2 parentSize = parentRect.GetSize();
    Vec2 parentPivot = GetParentPivot();

    RectTransformRect rect;
    rect.min = (_anchorMin - parentPivot) * parentSize + _offsetMin;
    rect.max = (_anchorMax - parentPivot) * parentSize + _offsetMax;
    return rect;
}

Vec2 RectTransform::GetAnchoredPosition() const
{
    RectTransformRect rect = GetRect();
    Vec2 size = rect.GetSize();
    Vec2 pivotPosition = rect.min + size * _pivot;
    Vec2 anchorCenter = (_anchorMin + _anchorMax) * 0.5f;
    Vec2 parentSize = GetParentRect().GetSize();
    Vec2 parentPivot = GetParentPivot();

    return pivotPosition - (anchorCenter - parentPivot) * parentSize;
}

void RectTransform::SetAnchoredPosition(const Vec2& anchoredPosition)
{
    Vec2 delta = anchoredPosition - GetAnchoredPosition();
    _offsetMin += delta;
    _offsetMax += delta;
    ApplyToTransform();
}

void RectTransform::SetSize(const Vec2& size)
{
    RectTransformRect rect = GetRect();
    Vec2 currentSize = rect.GetSize();
    Vec2 delta = size - currentSize;

    _offsetMin -= delta * _pivot;
    _offsetMax += delta * (Vec2(1.f, 1.f) - _pivot);
    ApplyToTransform();
}

void RectTransform::ApplyToTransform()
{
    RectTransformRect rect = GetRect();
    Vec2 size = rect.GetSize();
    Vec2 pivotPosition = rect.min + size * _pivot;
    Vec3 parentScale = GetParent() != nullptr ? GetParent()->GetScale() : Vec3(1.f, 1.f, 1.f);

    Vec3 localPos = Vec3(pivotPosition.x / parentScale.x, pivotPosition.y / parentScale.y, GetLocalPosition().z);
    SetLocalPosition(localPos);

    Vec3 scaledSize = Vec3(size.x / parentScale.x, size.y / parentScale.y, GetLocalScale().z);
    SetLocalScale(scaledSize);
}

RectTransformRect RectTransform::GetParentRect() const
{
    Transform* parent = GetParent();
    RectTransform* parentRectTransform = dynamic_cast<RectTransform*>(parent);
    if (parentRectTransform != nullptr)
        return parentRectTransform->GetRect();

    const float width = GAME->GetGameDesc().sceneWidth;
    const float height = GAME->GetGameDesc().sceneHeight;

    RectTransformRect rect;
    rect.min = Vec2(-width * 0.5f, -height * 0.5f);
    rect.max = Vec2(width * 0.5f, height * 0.5f);
    return rect;
}

Vec2 RectTransform::GetParentPivot() const
{
    Transform* parent = GetParent();
    RectTransform* parentRectTransform = dynamic_cast<RectTransform*>(parent);
    if (parentRectTransform != nullptr)
        return parentRectTransform->GetPivot();

    return Vec2(0.5f, 0.5f);
}