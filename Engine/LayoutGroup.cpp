#include "pch.h"
#include "LayoutGroup.h"
#include "GameObject.h"
#include "OnGUIUtils.h"
#include "RectTransform.h"

namespace
{
    const char* const LayoutChildAlignmentNames[] =
    {
        "Upper Left",
        "Upper Center",
        "Upper Right",
        "Middle Left",
        "Middle Center",
        "Middle Right",
        "Lower Left",
        "Lower Center",
        "Lower Right",
    };

}

LayoutGroup::LayoutGroup(ComponentType type)
    : Super(type)
{
}

LayoutGroup::~LayoutGroup()
{
}

void LayoutGroup::Awake()
{
    Super::Awake();
    SetDirty();
}

void LayoutGroup::Update()
{
    RebuildLayout();
}

bool LayoutGroup::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();

    RECT padding = _padding;
    if (OnGUIUtils::DrawRect("Padding", &padding))
    {
        SetPadding(padding);
        changed = true;
    }

    Vec2 spacing = _spacing;
    if (OnGUIUtils::DrawVec2("Spacing", &spacing, 1.f))
    {
        SetSpacing(spacing);
        changed = true;
    }

    LayoutChildAlignment alignment = _childAlignment;
    if (OnGUIUtils::DrawEnumCombo("Child Alignment", alignment, LayoutChildAlignmentNames, _countof(LayoutChildAlignmentNames)))
    {
        SetChildAlignment(alignment);
        changed = true;
    }

    bool controlChildWidth = _controlChildWidth;
    if (OnGUIUtils::DrawBool("Control Child Width", &controlChildWidth))
    {
        SetControlChildWidth(controlChildWidth);
        changed = true;
    }

    bool controlChildHeight = _controlChildHeight;
    if (OnGUIUtils::DrawBool("Control Child Height", &controlChildHeight))
    {
        SetControlChildHeight(controlChildHeight);
        changed = true;
    }

    if (ImGui::Button("Rebuild Layout"))
    {
        SetDirty();
        RebuildLayout();
        changed = true;
    }

    return changed;
}

void LayoutGroup::RebuildLayout()
{
    RectTransform* rectTransform = dynamic_cast<RectTransform*>(GetTransform());
    if (rectTransform == nullptr)
        return;

    vector<RectTransform*> children = GetChildRectTransforms();
    LayoutChildren(rectTransform, children);
    _dirty = false;
}

vector<RectTransform*> LayoutGroup::GetChildRectTransforms()
{
    vector<RectTransform*> result;
    Transform* transform = GetTransform();
    if (transform == nullptr)
        return result;

    const vector<TransformRef>& children = transform->GetChildren();
    result.reserve(children.size());
    for (const TransformRef& childRef : children)
    {
        Transform* child = childRef.Resolve();
        RectTransform* rectTransform = dynamic_cast<RectTransform*>(child);
        if (rectTransform != nullptr)
            result.push_back(rectTransform);
    }
    return result;
}

Vec2 LayoutGroup::GetInnerSize(const Vec2& size) const
{
    return Vec2(
        max(0.f, size.x - static_cast<float>(_padding.left + _padding.right)),
        max(0.f, size.y - static_cast<float>(_padding.top + _padding.bottom)));
}

Vec2 LayoutGroup::GetAlignmentFactor() const
{
    switch (_childAlignment)
    {
    case LayoutChildAlignment::UpperLeft: return Vec2(0.f, 0.f);
    case LayoutChildAlignment::UpperCenter: return Vec2(0.5f, 0.f);
    case LayoutChildAlignment::UpperRight: return Vec2(1.f, 0.f);
    case LayoutChildAlignment::MiddleLeft: return Vec2(0.f, 0.5f);
    case LayoutChildAlignment::MiddleCenter: return Vec2(0.5f, 0.5f);
    case LayoutChildAlignment::MiddleRight: return Vec2(1.f, 0.5f);
    case LayoutChildAlignment::LowerLeft: return Vec2(0.f, 1.f);
    case LayoutChildAlignment::LowerCenter: return Vec2(0.5f, 1.f);
    case LayoutChildAlignment::LowerRight: return Vec2(1.f, 1.f);
    default: return Vec2(0.f, 0.f);
    }
}

float LayoutGroup::GetAlignedOffset(float available, float content, float factor) const
{
    return max(0.f, available - content) * factor;
}

void LayoutGroup::SetChildRect(RectTransform* child, const Vec2& positionFromTopLeft, const Vec2& size) const
{
    if (child == nullptr)
        return;

    child->SetAnchors(Vec2(0.f, 1.f), Vec2(0.f, 1.f));
    child->SetSize(size);

    const Vec2 pivot = child->GetPivot();
    const Vec2 pivotPositionFromTopLeft = positionFromTopLeft + size * pivot;
    child->SetAnchoredPosition(Vec2(pivotPositionFromTopLeft.x, -pivotPositionFromTopLeft.y));
}
