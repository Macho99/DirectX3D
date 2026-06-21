#include "pch.h"
#include "ScrollView.h"
#include "OnGUIUtils.h"
#include <cmath>

ScrollView::ScrollView()
    : Super(StaticType)
{
    _mesh = RESOURCES->GetQuadMesh();
    _maskMode = UIMaskMode::VisibleMask;
    _material = RESOURCES->AllocateUIDefaultMaterial();
}

void ScrollView::Awake()
{
    GameObjectRef contentRef = CUR_SCENE->Add("Content", GetTransform());
    RectTransform* contentRect = static_cast<RectTransform*>(contentRef.Resolve()->GetTransform());
    _contentRectTransformRef = RectTransformRef(contentRect);
}

void ScrollView::Update()
{
    Super::Update();

    if (_isDragging)
        return;

    RectTransform* contentRectTransform = _contentRectTransformRef.Resolve();
    if (contentRectTransform == nullptr)
        return;

    const float dt = DT;

    const Vec2 correction = CalculateBoundsCorrection(contentRectTransform);
    if (std::abs(correction.x) > 0.01f || std::abs(correction.y) > 0.01f)
    {
        const float t = std::min(_elasticReturnSpeed * dt, 1.0f);
        Vec2 delta = correction * t;

        if (std::abs(correction.x) < 0.5f)
            delta.x = correction.x;
        if (std::abs(correction.y) < 0.5f)
            delta.y = correction.y;

        contentRectTransform->MoveOffsets(delta);
        StopVelocityOnCorrectedAxes(correction);
        return;
    }

    if (_inertiaEnabled == false)
    {
        _velocity = Vec2(0.0f, 0.0f);
        return;
    }

    if (_horizontalScrollEnabled == false)
        _velocity.x = 0.0f;
    if (_verticalScrollEnabled == false)
        _velocity.y = 0.0f;

    if (_velocity.LengthSquared() < 1.0f)
    {
        _velocity = Vec2(0.0f, 0.0f);
        return;
    }

    contentRectTransform->MoveOffsets(_velocity * dt);
    const float damping = std::exp(-_inertiaDamping * dt);
    _velocity *= damping;
}

void ScrollView::OnBeginDrag()
{
    _isDragging = true;
    _velocity = Vec2(0.0f, 0.0f);
}

void ScrollView::OnDrag(DragEvent event)
{
    RectTransform* contentRectTransform = _contentRectTransformRef.Resolve();
    if (contentRectTransform == nullptr)
        return;

    if (_horizontalScrollEnabled == false)
        event.delta.x = 0.f;
    if (_verticalScrollEnabled == false)
        event.delta.y = 0.f;

    const Vec2 contentDelta = Vec2(event.delta.x, -event.delta.y);
    contentRectTransform->MoveOffsets(contentDelta);

    const float dt = DT;
    if (dt > 0.0f)
        _velocity = contentDelta / dt;
}

void ScrollView::OnEndDrag()
{
    _isDragging = false;
}

bool ScrollView::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();

    ImGui::Separator();
    changed |= OnGUIUtils::DrawComponentRef("Content RectTransform", _contentRectTransformRef);

    if (OnGUIUtils::DrawResourceRef("Background Texture", _backgroundTexture))
    {
        SetBackgroundTexture(_backgroundTexture);
        changed = true;
    }

    changed |= OnGUIUtils::DrawBool("Horizontal Scroll Enabled", &_horizontalScrollEnabled);
    changed |= OnGUIUtils::DrawBool("Vertical Scroll Enabled", &_verticalScrollEnabled);
    changed |= OnGUIUtils::DrawBool("Inertia Enabled", &_inertiaEnabled);
    changed |= OnGUIUtils::DrawFloat("Inertia Damping", &_inertiaDamping, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Elastic Return Speed", &_elasticReturnSpeed, 0.1f);

    return changed;
}

void ScrollView::SetBackgroundTexture(ResourceRef<Texture> texture)
{
    _backgroundTexture = texture;
    _material.Resolve()->SetDiffuseMap(texture);
}

Vec2 ScrollView::CalculateBoundsCorrection(RectTransform* contentRectTransform) const
{
    RectTransform* viewportRectTransform = dynamic_cast<RectTransform*>(const_cast<ScrollView*>(this)->GetTransform());
    if (viewportRectTransform == nullptr)
        return Vec2(0.0f, 0.0f);

    const Vec2 viewportSize = viewportRectTransform->GetSize();
    const Vec2 viewportMin = viewportSize * -0.5f;
    const Vec2 viewportMax = viewportSize * 0.5f;
    const RectTransformRect contentRect = contentRectTransform->GetRect();

    Vec2 correction = Vec2(0.0f, 0.0f);
    if (_horizontalScrollEnabled)
        correction.x = CalculateAxisCorrection(contentRect.min.x, contentRect.max.x, viewportMin.x, viewportMax.x);
    if (_verticalScrollEnabled)
        correction.y = CalculateAxisCorrection(contentRect.min.y, contentRect.max.y, viewportMin.y, viewportMax.y);

    return correction;
}

float ScrollView::CalculateAxisCorrection(float contentMin, float contentMax, float viewportMin, float viewportMax) const
{
    const float contentSize = contentMax - contentMin;
    const float viewportSize = viewportMax - viewportMin;

    if (contentSize <= viewportSize)
    {
        if (contentMin < viewportMin)
            return viewportMin - contentMin;
        if (contentMax > viewportMax)
            return viewportMax - contentMax;
    }
    else
    {
        if (contentMin > viewportMin)
            return viewportMin - contentMin;
        if (contentMax < viewportMax)
            return viewportMax - contentMax;
    }

    return 0.0f;
}

void ScrollView::StopVelocityOnCorrectedAxes(const Vec2& correction)
{
    if (std::abs(correction.x) > 0.01f)
        _velocity.x = 0.0f;
    if (std::abs(correction.y) > 0.01f)
        _velocity.y = 0.0f;
}
