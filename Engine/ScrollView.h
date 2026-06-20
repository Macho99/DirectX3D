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

    virtual void OnBeginDrag() override;
    virtual void OnDrag(DragEvent event) override;
    virtual void OnEndDrag() override;

    virtual bool OnGUI() override;

    RectTransformRef GetContentRectTransformRef() const { return _contentRectTransformRef; }
    void SetBackgroundTexture(ResourceRef<Texture> texture);

private:
    RectTransformRef _contentRectTransformRef;
    ResourceRef<Texture> _backgroundTexture;
    bool _horizontalScrollEnabled = false;
    bool _verticalScrollEnabled = true;
};

