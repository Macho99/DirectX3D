#include "pch.h"
#include "PlayerAnimEventHandler.h"
#include "AnimationImportSetting.h"
#include "TrailRenderer.h"
#include "ModelAnimator.h"
#include "OnGUIUtils.h"

void PlayerAnimEventHandler::Start()
{
    TrailRenderer* trailRenderer = GetGameObject()->GetComponentInChildren<TrailRenderer>();
    ASSERT(trailRenderer != nullptr, "TrailRenderer is null");
    _trailRenderer = trailRenderer;

    _audioSource = GetGameObject()->GetComponent<AudioSource>();
    if (_audioSource.Resolve() == nullptr)
    {
        _audioSource = GetGameObject()->AddComponent<AudioSource>();
    }

    ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\TrailMat.mat");
    trailRenderer->SetMaterial(materialRef);
}

void PlayerAnimEventHandler::Update()
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
        }
    }

    UpdateTrailRenderer();
}

void PlayerAnimEventHandler::OnAnimationEvent(const AnimationEvent& animationEvent)
{
    if (animationEvent.eventName == "Impulse")
    {
        ModelAnimator* animator = GetGameObject()->GetModelAnimator();
        if (animator == nullptr)
            return;

        Quaternion dummyRot;
        Vec3 dummyScale, swordPosition;
        Matrix swordEndMatrix;
        animator->TryGetModelSocketWorldMatrix("SwordStart", swordEndMatrix);
        swordEndMatrix.Decompose(OUT dummyScale, OUT dummyRot, OUT swordPosition);
        GM->PlayImpulse(swordPosition);

        AudioSource* audioSource = _audioSource.Resolve();
        if (audioSource != nullptr)
        {
            audioSource->SetRandomClipAndPlay(_impulseAudioClips);
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
            audioSource->SetRandomClipAndPlay(_swordAudioClips);
        }
    }
    else
    {
        _trailUpdateTime = FLT_MAX;
        _trailAnimationIndex = -1;
    }

    UpdateTrailRenderer(true);
}

bool PlayerAnimEventHandler::OnGUI()
{
    bool changed = Super::OnGUI();
    changed |= OnGUIUtils::DrawResourceRefVector("Sword Audio Clips", _swordAudioClips);
    changed |= OnGUIUtils::DrawResourceRefVector("Impulse Audio Clips", _impulseAudioClips);

    return changed;
}

void PlayerAnimEventHandler::UpdateTrailRenderer(bool force)
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
    animator->TryGetModelSocketWorldMatrix("SwordStart", swordStartMatrix);
    swordStartMatrix.Decompose(OUT dummyScale, OUT dummyRot, OUT startPosition);

    Vec3 endPosition;
    Matrix swordEndMatrix;
    animator->TryGetModelSocketWorldMatrix("SwordEnd", swordEndMatrix);
    swordEndMatrix.Decompose(OUT dummyScale, OUT dummyRot, OUT endPosition);

    trailRenderer->AddPoint(startPosition, endPosition);
}
