#include "pch.h"
#include "ZombieAnimEventHandler.h"
#include "AnimationImportSetting.h"
#include "TrailRenderer.h"
#include "ModelAnimator.h"
#include "MeshRenderer.h"
#include "ModelSocketFollower.h"
#include "OnGUIUtils.h"

void ZombieAnimEventHandler::Start()
{
    TrailRenderer* trailRenderer = GetGameObject()->GetComponentInChildren<TrailRenderer>();
    ASSERT(trailRenderer != nullptr, "TrailRenderer is null");
    _trailRenderer = trailRenderer;

    MeshRenderer* screamRenderer = GetGameObject()->GetComponentInChildren<MeshRenderer>();
    ASSERT(screamRenderer != nullptr, "Scream MeshRenderer is null");
    _screamRenderer = screamRenderer;

    screamRenderer->GetGameObject()->GetComponent<ModelSocketFollower>()->SetModelAnimator(GetGameObject()->GetModelAnimator());
    screamRenderer->GetGameObject()->SetActive(false);

    ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\TrailMat.mat");
    trailRenderer->SetMaterial(materialRef);

    _audioSource = GetGameObject()->GetComponent<AudioSource>();
    if (_audioSource.Resolve() == nullptr)
    {
        _audioSource = GetGameObject()->AddComponent<AudioSource>();
    }
}

void ZombieAnimEventHandler::Update()
{
    ModelAnimator* animator = GetGameObject()->GetModelAnimator();
    if (_trailUpdateTime != FLT_MAX && animator != nullptr)
    {
        const TweenDesc& tweenDesc = animator->GetTweenDesc();
        int32 animationIndex = tweenDesc.cur.GetSingleAnimationIndex();
        if (tweenDesc.next.HasAnimation())
            animationIndex = tweenDesc.next.GetSingleAnimationIndex();

        if (animationIndex != _trailAnimationIndex)
        {
            _trailUpdateTime = FLT_MAX;
            _trailAnimationIndex = -1;

            MeshRenderer* screamRenderer = _screamRenderer.Resolve();
            if (screamRenderer != nullptr)
            {
                screamRenderer->GetGameObject()->SetActive(false);
            }
        }
    }

    UpdateTrailRenderer();
}

void ZombieAnimEventHandler::OnAnimationEvent(const AnimationEvent& animationEvent)
{
    if (animationEvent.eventName == "Scream")
    {
        MeshRenderer* screamRenderer = _screamRenderer.Resolve();
        if (screamRenderer != nullptr)
        {
            screamRenderer->GetGameObject()->SetActive(animationEvent.boolParam);
        }

        AudioSource* audioSource = _audioSource.Resolve();
        if (audioSource != nullptr)
        {
            audioSource->SetRandomClipAndPlay(_screamAudioClips);
        }
    }
    else if (animationEvent.eventName != "Attack")
        return;

    TrailRenderer* trailRenderer = _trailRenderer.Resolve();
    if (animationEvent.boolParam == true)
    {
        trailRenderer->ClearPoints();
        _trailUpdateTime = TIME->GetGameTime();

        ModelAnimator* animator = GetGameObject()->GetModelAnimator();
        if (animator != nullptr)
        {
            const TweenDesc& tweenDesc = animator->GetTweenDesc();
            _trailAnimationIndex = tweenDesc.next.GetSingleAnimationIndex();
            if (_trailAnimationIndex < 0)
                _trailAnimationIndex = tweenDesc.cur.GetSingleAnimationIndex();
        }
        AudioSource* audioSource = _audioSource.Resolve();
        if (audioSource != nullptr)
        {
            audioSource->SetRandomClipAndPlay(_attackAudioClips);
        }
    }
    else
    {
        _trailUpdateTime = FLT_MAX;
        _trailAnimationIndex = -1;
    }

    UpdateTrailRenderer(true);
}

bool ZombieAnimEventHandler::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= OnGUIUtils::DrawResourceRefVector("Scream Audio Clips", _screamAudioClips);
    changed |= OnGUIUtils::DrawResourceRefVector("Attack Audio Clips", _attackAudioClips);
    return changed;
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
