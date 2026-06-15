#pragma once
#include "LayoutGroup.h"

class VerticalLayoutGroup : public LayoutGroup
{
    using Super = LayoutGroup;
    DECLARE_COMPONENT(VerticalLayoutGroup)

public:
    VerticalLayoutGroup();
    virtual ~VerticalLayoutGroup();

protected:
    virtual void LayoutChildren(RectTransform* rectTransform, const vector<RectTransform*>& children) override;
};
