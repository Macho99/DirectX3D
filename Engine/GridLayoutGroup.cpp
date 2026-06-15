#include "pch.h"
#include "GridLayoutGroup.h"
#include "OnGUIUtils.h"
#include "RectTransform.h"

namespace
{
    const char* const GridStartCornerNames[] =
    {
        "Upper Left",
        "Upper Right",
        "Lower Left",
        "Lower Right",
    };

    const char* const GridStartAxisNames[] =
    {
        "Horizontal",
        "Vertical",
    };

    const char* const GridConstraintNames[] =
    {
        "Flexible",
        "Fixed Column Count",
        "Fixed Row Count",
    };
}

GridLayoutGroup::GridLayoutGroup()
    : Super(StaticType)
{
    _controlChildWidth = true;
    _controlChildHeight = true;
}

GridLayoutGroup::~GridLayoutGroup()
{
}

bool GridLayoutGroup::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();

    Vec2 cellSize = _cellSize;
    if (OnGUIUtils::DrawVec2("Cell Size", &cellSize, 1.f))
    {
        _cellSize = Vec2(max(0.f, cellSize.x), max(0.f, cellSize.y));
        SetDirty();
        changed = true;
    }

    GridLayoutGroupStartCorner startCorner = _startCorner;
    if (OnGUIUtils::DrawEnumCombo("Start Corner", startCorner, GridStartCornerNames, _countof(GridStartCornerNames)))
    {
        _startCorner = startCorner;
        SetDirty();
        changed = true;
    }

    GridLayoutGroupStartAxis startAxis = _startAxis;
    if (OnGUIUtils::DrawEnumCombo("Start Axis", startAxis, GridStartAxisNames, _countof(GridStartAxisNames)))
    {
        _startAxis = startAxis;
        SetDirty();
        changed = true;
    }

    GridLayoutGroupConstraint constraint = _constraint;
    if (OnGUIUtils::DrawEnumCombo("Constraint", constraint, GridConstraintNames, _countof(GridConstraintNames)))
    {
        _constraint = constraint;
        SetDirty();
        changed = true;
    }

    int constraintCount = _constraintCount;
    if (OnGUIUtils::DrawInt32("Constraint Count", &constraintCount, 1.f))
    {
        _constraintCount = max(1, constraintCount);
        SetDirty();
        changed = true;
    }

    return changed;
}

void GridLayoutGroup::LayoutChildren(RectTransform* rectTransform, const vector<RectTransform*>& children)
{
    if (rectTransform == nullptr || children.empty())
        return;

    Vec2 innerSize = GetInnerSize(rectTransform->GetSize());
    Vec2 alignment = GetAlignmentFactor();
    const int childCount = static_cast<int>(children.size());
    const int columns = GetColumnCount(childCount, innerSize);
    const int rows = GetRowCount(childCount, columns);
    const Vec2 cellSize(max(0.f, _cellSize.x), max(0.f, _cellSize.y));
    const Vec2 spacing(max(0.f, _spacing.x), max(0.f, _spacing.y));

    Vec2 contentSize(
        static_cast<float>(columns) * cellSize.x + static_cast<float>(max(0, columns - 1)) * spacing.x,
        static_cast<float>(rows) * cellSize.y + static_cast<float>(max(0, rows - 1)) * spacing.y);

    Vec2 start(
        static_cast<float>(_padding.left) + GetAlignedOffset(innerSize.x, contentSize.x, alignment.x),
        static_cast<float>(_padding.top) + GetAlignedOffset(innerSize.y, contentSize.y, alignment.y));

    for (int i = 0; i < childCount; ++i)
    {
        int column = 0;
        int row = 0;
        if (_startAxis == GridLayoutGroupStartAxis::Horizontal)
        {
            column = i % columns;
            row = i / columns;
        }
        else
        {
            column = i / rows;
            row = i % rows;
        }

        if (_startCorner == GridLayoutGroupStartCorner::UpperRight || _startCorner == GridLayoutGroupStartCorner::LowerRight)
            column = columns - 1 - column;
        if (_startCorner == GridLayoutGroupStartCorner::LowerLeft || _startCorner == GridLayoutGroupStartCorner::LowerRight)
            row = rows - 1 - row;

        Vec2 position(
            start.x + static_cast<float>(column) * (cellSize.x + spacing.x),
            start.y + static_cast<float>(row) * (cellSize.y + spacing.y));

        SetChildRect(children[i], position, cellSize);
    }
}

int GridLayoutGroup::GetColumnCount(int childCount, const Vec2& innerSize) const
{
    if (childCount <= 0)
        return 1;

    if (_constraint == GridLayoutGroupConstraint::FixedColumnCount)
        return max(1, _constraintCount);

    if (_constraint == GridLayoutGroupConstraint::FixedRowCount)
        return max(1, static_cast<int>(ceilf(static_cast<float>(childCount) / static_cast<float>(max(1, _constraintCount)))));

    const float stride = max(1.f, _cellSize.x + max(0.f, _spacing.x));
    return max(1, min(childCount, static_cast<int>(floorf((innerSize.x + max(0.f, _spacing.x)) / stride))));
}

int GridLayoutGroup::GetRowCount(int childCount, int columnCount) const
{
    if (childCount <= 0)
        return 1;

    if (_constraint == GridLayoutGroupConstraint::FixedRowCount)
        return max(1, _constraintCount);

    return max(1, static_cast<int>(ceilf(static_cast<float>(childCount) / static_cast<float>(max(1, columnCount)))));
}
