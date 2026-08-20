#include "pch.h"
#include "ZombieAnimEventHandler.h"
#include "AnimationImportSetting.h"
#include "TrailRenderer.h"
#include "ModelAnimator.h"

void ZombieAnimEventHandler::Start()
{
    TrailRenderer* trailRenderer = GetGameObject()->GetComponentInChildren<TrailRenderer>();
    ASSERT(trailRenderer != nullptr, "TrailRenderer is null");
    _trailRenderer = trailRenderer;

    ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\TrailMat.mat");
    trailRenderer->SetMaterial(materialRef);
}

void ZombieAnimEventHandler::Update()
{
    UpdateTrailRenderer();
}

void ZombieAnimEventHandler::OnAnimationEvent(const AnimationEvent& animationEvent)
{
    if (animationEvent.eventName != "Attack")
        return;

    TrailRenderer* trailRenderer = _trailRenderer.Resolve();
    if (animationEvent.boolParam == true)
    {
        trailRenderer->ClearPoints();
        _trailUpdateTime = TIME->GetGameTime();
    }
    else
    {
        _trailUpdateTime = FLT_MAX;
    }

    UpdateTrailRenderer(true);
}

void ZombieAnimEventHandler::UpdateTrailRenderer(bool force)
{
    if (force == false)
    {
        const float gameTime = TIME->GetGameTime();
        if (gameTime <= _trailUpdateTime + 0.001f)
        {
            return;
        }
    }

    TrailRenderer* trailRenderer = _trailRenderer.Resolve();
    ModelAnimator* animator = GetGameObject()->GetModelAnimator();
    if (animator == nullptr || trailRenderer == nullptr)
        return;

    Vec3 dummyScale;
    Quaternion dummyRot;

    Vec3 startPosition;
    Matrix swordStartMatrix;
    animator->TryGetModelSocketWorldMatrix("HandStart", swordStartMatrix);
    swordStartMatrix.Decompose(OUT dummyScale, OUT dummyRot, OUT startPosition);

    Vec3 endPosition;
    Matrix swordEndMatrix;
    animator->TryGetModelSocketWorldMatrix("HandEnd", swordEndMatrix);
    swordEndMatrix.Decompose(OUT dummyScale, OUT dummyRot, OUT endPosition);

    trailRenderer->AddPoint(startPosition, endPosition);
}
