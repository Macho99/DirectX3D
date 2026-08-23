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
    MeshRenderer* screamRenderer = _screamRenderer.Resolve();
    TrailRenderer* trailRenderer = _trailRenderer.Resolve();

    if (animator == nullptr || screamRenderer == nullptr || trailRenderer == nullptr)
        return;

    const TweenDesc& tweenDesc = animator->GetTweenDesc();
    int32 animationIndex = tweenDesc.cur.GetSingleAnimationIndex();
    if (tweenDesc.next.HasAnimation())
        animationIndex = tweenDesc.next.GetSingleAnimationIndex();

    if (animationIndex != _curAnimationIndex)
    {
        screamRenderer->GetGameObject()->SetActive(false);

        if (_isAttackEnd == false)
        {
            trailRenderer->ClearPoints();
            _trailUpdateTime = FLT_MAX;
        }
    }

    UpdateTrailRenderer();
}

void ZombieAnimEventHandler::OnAnimationEvent(const AnimationEvent& animationEvent)
{
    ModelAnimator* animator = GetGameObject()->GetModelAnimator();
    MeshRenderer* screamRenderer = _screamRenderer.Resolve();
    AudioSource* audioSource = _audioSource.Resolve();
    TrailRenderer* trailRenderer = _trailRenderer.Resolve();

    if (animator == nullptr || screamRenderer == nullptr || audioSource == nullptr || trailRenderer == nullptr)
        return;


    const TweenDesc& tweenDesc = animator->GetTweenDesc();
    _curAnimationIndex = tweenDesc.cur.GetSingleAnimationIndex();

    if (animationEvent.eventName == "Scream")
    {
        screamRenderer->GetGameObject()->SetActive(animationEvent.boolParam);
        audioSource->SetRandomClipAndPlay(_screamAudioClips);
    }
    else if (animationEvent.eventName == "Attack")
    {
        _isAttackEnd = !animationEvent.boolParam;
        if (animationEvent.boolParam == true)
        {
            trailRenderer->ClearPoints();
            _trailUpdateTime = TIME->GetGameTime();

            audioSource->SetRandomClipAndPlay(_attackAudioClips);
        }
        else
        {
            _trailUpdateTime = FLT_MAX;
            _curAnimationIndex = -1;
        }
        UpdateTrailRenderer(true);
    }
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
