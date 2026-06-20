#pragma once
#include "Transform.h"

struct RectTransformRect
{
    Vec2 min = Vec2(0.f, 0.f);
    Vec2 max = Vec2(0.f, 0.f);

    Vec2 GetSize() const { return max - min; }
    Vec2 GetCenter() const { return (min + max) * 0.5f; }
};

class RectTransform : public Transform
{
    using Super = Transform;
public:
    static constexpr ComponentType StaticType = ComponentType::Transform;
    static const char* StaticName() { return "RectTransform"; }
    static std::unique_ptr<Component> CreateInstance()
    {
        return std::make_unique<RectTransform>();
    }
    static void EnsureAutoRegister() { (void)_autoRegister; }

private:
    struct AutoRegister
    {
        AutoRegister()
        {
            ComponentRegistry::Get().Register(
                StaticType,
                StaticName(),
                &RectTransform::CreateInstance);
        }
    };
    inline static AutoRegister _autoRegister;

public:
    RectTransform();
    virtual ~RectTransform();

    virtual void Awake() override;
    virtual bool OnGUI() override;

    const Vec2& GetAnchorMin() const { return _anchorMin; }
    const Vec2& GetAnchorMax() const { return _anchorMax; }
    void SetAnchorMin(const Vec2& anchorMin);
    void SetAnchorMax(const Vec2& anchorMax);
    void SetAnchors(const Vec2& anchorMin, const Vec2& anchorMax);

    const Vec2& GetOffsetMin() const { return _offsetMin; }
    const Vec2& GetOffsetMax() const { return _offsetMax; }
    void SetOffsetMin(const Vec2& offsetMin);
    void SetOffsetMax(const Vec2& offsetMax);
    void SetOffsets(const Vec2& offsetMin, const Vec2& offsetMax);
    void MoveOffsets(const Vec2& delta);

    const Vec2& GetPivot() const { return _pivot; }

    RectTransformRect GetRect() const;
    Vec2 GetSize() const { return GetRect().GetSize(); }
    Vec2 GetAnchoredPosition() const;
    void SetAnchoredPosition(const Vec2& anchoredPosition);
    void SetSize(const Vec2& size);

    void ApplyToTransform();

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_anchorMin));
        ar(CEREAL_NVP(_anchorMax));
        ar(CEREAL_NVP(_offsetMin));
        ar(CEREAL_NVP(_offsetMax));

        if (Archive::is_loading::value)
            ApplyToTransform();
    }

private:
    RectTransformRect GetParentRect() const;
    Vec2 GetParentPivot() const;

private:
    Vec2 _anchorMin = Vec2(0.5f, 0.5f);
    Vec2 _anchorMax = Vec2(0.5f, 0.5f);
    Vec2 _offsetMin = Vec2(-50.f, -50.f);
    Vec2 _offsetMax = Vec2(50.f, 50.f);
    const Vec2 _pivot = Vec2(0.5f, 0.5f);
};