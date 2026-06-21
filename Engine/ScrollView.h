#pragma once
#include "IDragHandler.h"
#include "UIRenderer.h"

class ScrollView : public UIRenderer, public IDragHandler
{
    using Super = UIRenderer;
    DECLARE_COMPONENT(ScrollView)
public:
    ScrollView();

    virtual void Awake() override;
    virtual void Update() override;

    virtual void OnBeginDrag() override;
    virtual void OnDrag(DragEvent event) override;
    virtual void OnEndDrag() override;

    virtual bool OnGUI() override;
    virtual int GetVersion() const override { return 1; }

    RectTransformRef GetContentRectTransformRef() const { return _contentRectTransformRef; }
    void SetBackgroundTexture(ResourceRef<Texture> texture);

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

        if (Archive::is_saving::value || _version >= 1)
        {
            ar(CEREAL_NVP(_contentRectTransformRef));
            ar(CEREAL_NVP(_backgroundTexture));
            ar(CEREAL_NVP(_horizontalScrollEnabled));
            ar(CEREAL_NVP(_verticalScrollEnabled));
            ar(CEREAL_NVP(_inertiaEnabled));
            ar(CEREAL_NVP(_inertiaDamping));
            ar(CEREAL_NVP(_elasticReturnSpeed));
        }

        if (Archive::is_loading::value)
            SetBackgroundTexture(_backgroundTexture);
    }

private:
    Vec2 CalculateBoundsCorrection(RectTransform* contentRectTransform) const;
    float CalculateAxisCorrection(float contentMin, float contentMax, float viewportMin, float viewportMax) const;
    void StopVelocityOnCorrectedAxes(const Vec2& correction);

private:
    RectTransformRef _contentRectTransformRef;
    ResourceRef<Texture> _backgroundTexture;
    bool _horizontalScrollEnabled = false;
    bool _verticalScrollEnabled = true;
    bool _inertiaEnabled = true;
    float _inertiaDamping = 8.0f;
    float _elasticReturnSpeed = 12.0f;

    bool _isDragging = false;
    Vec2 _velocity = Vec2(0.0f, 0.0f);
};
