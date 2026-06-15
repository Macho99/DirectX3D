#pragma once
#include "LayoutGroup.h"

class HorizontalLayoutGroup : public LayoutGroup
{
    using Super = LayoutGroup;
    DECLARE_COMPONENT(HorizontalLayoutGroup)

public:
    HorizontalLayoutGroup();
    virtual ~HorizontalLayoutGroup();

protected:
    virtual void LayoutChildren(RectTransform* rectTransform, const vector<RectTransform*>& children) override;
};
