#pragma once
#include "Component.h"

enum class LayoutChildAlignment : uint8
{
    UpperLeft,
    UpperCenter,
    UpperRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    LowerLeft,
    LowerCenter,
    LowerRight,
};

class LayoutGroup : public Component
{
    using Super = Component;

public:
    LayoutGroup(ComponentType type);
    virtual ~LayoutGroup();

    virtual void Awake() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    const RECT& GetPadding() const { return _padding; }
    void SetPadding(const RECT& padding) { _padding = padding; SetDirty(); }

    Vec2 GetSpacing() const { return _spacing; }
    void SetSpacing(const Vec2& spacing) { _spacing = spacing; SetDirty(); }

    LayoutChildAlignment GetChildAlignment() const { return _childAlignment; }
    void SetChildAlignment(LayoutChildAlignment alignment) { _childAlignment = alignment; SetDirty(); }

    bool GetControlChildWidth() const { return _controlChildWidth; }
    void SetControlChildWidth(bool value) { _controlChildWidth = value; SetDirty(); }

    bool GetControlChildHeight() const { return _controlChildHeight; }
    void SetControlChildHeight(bool value) { _controlChildHeight = value; SetDirty(); }

    void SetDirty() { _dirty = true; }
    void RebuildLayout();

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_padding));
        ar(CEREAL_NVP(_spacing));
        ar(CEREAL_NVP(_childAlignment));
        ar(CEREAL_NVP(_controlChildWidth));
        ar(CEREAL_NVP(_controlChildHeight));

        if (Archive::is_loading::value)
            SetDirty();
    }

protected:
    virtual void LayoutChildren(class RectTransform* rectTransform, const vector<class RectTransform*>& children) = 0;

    vector<class RectTransform*> GetChildRectTransforms();
    Vec2 GetInnerSize(const Vec2& size) const;
    Vec2 GetAlignmentFactor() const;
    float GetAlignedOffset(float available, float content, float factor) const;
    void SetChildRect(class RectTransform* child, const Vec2& positionFromTopLeft, const Vec2& size) const;

protected:
    RECT _padding = { 0, 0, 0, 0 };
    Vec2 _spacing = Vec2(0.f, 0.f);
    LayoutChildAlignment _childAlignment = LayoutChildAlignment::UpperLeft;
    bool _controlChildWidth = true;
    bool _controlChildHeight = false;

private:
    bool _dirty = true;
};
