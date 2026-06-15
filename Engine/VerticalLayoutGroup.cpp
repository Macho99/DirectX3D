#include "pch.h"
#include "VerticalLayoutGroup.h"
#include "RectTransform.h"

VerticalLayoutGroup::VerticalLayoutGroup()
    : Super(StaticType)
{
    _controlChildWidth = true;
    _controlChildHeight = false;
}

VerticalLayoutGroup::~VerticalLayoutGroup()
{
}

void VerticalLayoutGroup::LayoutChildren(RectTransform* rectTransform, const vector<RectTransform*>& children)
{
    if (rectTransform == nullptr || children.empty())
        return;

    Vec2 innerSize = GetInnerSize(rectTransform->GetSize());
    Vec2 alignment = GetAlignmentFactor();
    const float spacing = max(0.f, _spacing.y);

    float contentHeight = 0.f;
    const float controlledHeight = max(0.f, (innerSize.y - spacing * static_cast<float>(children.size() - 1)) / static_cast<float>(children.size()));
    for (RectTransform* child : children)
    {
        Vec2 childSize = child->GetSize();
        if (_controlChildWidth)
            childSize.x = innerSize.x;
        if (_controlChildHeight)
            childSize.y = controlledHeight;

        contentHeight += childSize.y;
    }
    contentHeight += spacing * static_cast<float>(children.size() - 1);

    float y = static_cast<float>(_padding.top) + GetAlignedOffset(innerSize.y, contentHeight, alignment.y);
    for (RectTransform* child : children)
    {
        Vec2 childSize = child->GetSize();
        if (_controlChildWidth)
            childSize.x = innerSize.x;
        if (_controlChildHeight)
            childSize.y = controlledHeight;

        float x = static_cast<float>(_padding.left) + GetAlignedOffset(innerSize.x, childSize.x, alignment.x);
        SetChildRect(child, Vec2(x, y), childSize);
        y += childSize.y + spacing;
    }
}
