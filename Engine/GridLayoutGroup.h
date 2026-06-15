#pragma once
#include "LayoutGroup.h"

enum class GridLayoutGroupStartCorner : uint8
{
    UpperLeft,
    UpperRight,
    LowerLeft,
    LowerRight,
};

enum class GridLayoutGroupStartAxis : uint8
{
    Horizontal,
    Vertical,
};

enum class GridLayoutGroupConstraint : uint8
{
    Flexible,
    FixedColumnCount,
    FixedRowCount,
};

class GridLayoutGroup : public LayoutGroup
{
    using Super = LayoutGroup;
    DECLARE_COMPONENT(GridLayoutGroup)

public:
    GridLayoutGroup();
    virtual ~GridLayoutGroup();

    virtual bool OnGUI() override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_cellSize));
        ar(CEREAL_NVP(_startCorner));
        ar(CEREAL_NVP(_startAxis));
        ar(CEREAL_NVP(_constraint));
        ar(CEREAL_NVP(_constraintCount));

        if (Archive::is_loading::value)
            SetDirty();
    }

protected:
    virtual void LayoutChildren(RectTransform* rectTransform, const vector<RectTransform*>& children) override;

private:
    int GetColumnCount(int childCount, const Vec2& innerSize) const;
    int GetRowCount(int childCount, int columnCount) const;

private:
    Vec2 _cellSize = Vec2(100.f, 100.f);
    GridLayoutGroupStartCorner _startCorner = GridLayoutGroupStartCorner::UpperLeft;
    GridLayoutGroupStartAxis _startAxis = GridLayoutGroupStartAxis::Horizontal;
    GridLayoutGroupConstraint _constraint = GridLayoutGroupConstraint::Flexible;
    int _constraintCount = 2;
};
