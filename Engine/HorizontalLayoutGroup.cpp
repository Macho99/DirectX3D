#include "pch.h"
#include "HorizontalLayoutGroup.h"
#include "RectTransform.h"

HorizontalLayoutGroup::HorizontalLayoutGroup()
    : Super(StaticType)
{
    _controlChildWidth = false;
    _controlChildHeight = true;
}

HorizontalLayoutGroup::~HorizontalLayoutGroup()
{
}

void HorizontalLayoutGroup::LayoutChildren(RectTransform* rectTransform, const vector<RectTransform*>& children)
{
    if (rectTransform == nullptr || children.empty())
        return;

    Vec2 innerSize = GetInnerSize(rectTransform->GetSize());
    Vec2 alignment = GetAlignmentFactor();
    const float spacing = max(0.f, _spacing.x);

    float contentWidth = 0.f;
    const float controlledWidth = max(0.f, (innerSize.x - spacing * static_cast<float>(children.size() - 1)) / static_cast<float>(children.size()));
    for (RectTransform* child : children)
    {
        Vec2 childSize = child->GetSize();
        if (_controlChildWidth)
            childSize.x = controlledWidth;
        if (_controlChildHeight)
            childSize.y = innerSize.y;

        contentWidth += childSize.x;
    }
    contentWidth += spacing * static_cast<float>(children.size() - 1);

    float x = static_cast<float>(_padding.left) + GetAlignedOffset(innerSize.x, contentWidth, alignment.x);
    for (RectTransform* child : children)
    {
        Vec2 childSize = child->GetSize();
        if (_controlChildWidth)
            childSize.x = controlledWidth;
        if (_controlChildHeight)
            childSize.y = innerSize.y;

        float y = static_cast<float>(_padding.top) + GetAlignedOffset(innerSize.y, childSize.y, alignment.y);
        SetChildRect(child, Vec2(x, y), childSize);
        x += childSize.x + spacing;
    }
}
