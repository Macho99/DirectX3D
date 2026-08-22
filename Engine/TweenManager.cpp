#include "pch.h"
#include "TweenManager.h"
#include "Transform.h"
#include "UIImage.h"

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    float OutBounce(float value)
    {
        constexpr float n1 = 7.5625f;
        constexpr float d1 = 2.75f;
        if (value < 1.f / d1) return n1 * value * value;
        if (value < 2.f / d1)
        {
            value -= 1.5f / d1;
            return n1 * value * value + 0.75f;
        }
        if (value < 2.5f / d1)
        {
            value -= 2.25f / d1;
            return n1 * value * value + 0.9375f;
        }
        value -= 2.625f / d1;
        return n1 * value * value + 0.984375f;
    }
}

Tween::Tween(float duration, Initializer initializer, Applicator applicator)
    : _duration(max(0.f, duration)), _initializer(move(initializer)), _applicator(move(applicator))
{
}

Tween* Tween::SetEase(EaseType ease) { _ease = ease; return this; }
Tween* Tween::SetDelay(float delay) { _delay = max(0.f, delay); return this; }
Tween* Tween::SetLoops(int loops, LoopType loopType) { _loops = max(-1, loops); _loopType = loopType; return this; }
Tween* Tween::SetTimeScale(float timeScale) { _timeScale = max(0.f, timeScale); return this; }
Tween* Tween::SetTarget(const void* target) { _target = target; return this; }
Tween* Tween::OnStart(Callback callback) { _onStart = move(callback); return this; }
Tween* Tween::OnUpdate(UpdateCallback callback) { _onUpdate = move(callback); return this; }
Tween* Tween::OnComplete(Callback callback) { _onComplete = move(callback); return this; }
Tween* Tween::OnKill(Callback callback) { _onKill = move(callback); return this; }
Tween* Tween::Pause() { _paused = true; return this; }
Tween* Tween::Play() { if (!_completed && !_killed) _paused = false; return this; }

Tween* Tween::Restart()
{
    _elapsed = 0.f;
    _initialized = false;
    _started = false;
    _paused = false;
    _completed = false;
    _killed = false;
    return this;
}

Tween* Tween::Complete()
{
    if (_completed || _killed) return this;
    InitializeIfNeeded();
    if (!_started)
    {
        _started = true;
        if (_onStart) _onStart();
    }
    ApplyFinalValue();
    _completed = true;
    if (_onComplete) _onComplete();
    return this;
}

Tween* Tween::Kill(bool complete)
{
    if (_killed) return this;
    if (complete && !_completed) Complete();
    _killed = true;
    _paused = false;
    if (_onKill) _onKill();
    return this;
}

float Tween::GetElapsedPercentage() const
{
    if (_completed) return 1.f;
    if (_duration <= 0.f) return _started ? 1.f : 0.f;
    return clamp(max(0.f, _elapsed - _delay) / _duration, 0.f, 1.f);
}

void Tween::InitializeIfNeeded()
{
    if (_initialized) return;
    _initialized = true;
    if (_initializer) _initializer();
}

void Tween::ApplyFinalValue()
{
    const bool reverse = _loopType == LoopType::Yoyo && _loops >= 0 && (_loops % 2 == 1);
    const float progress = reverse ? 0.f : 1.f;
    if (_applicator) _applicator(EvaluateEase(_ease, progress));
    if (_onUpdate) _onUpdate(progress);
}

void Tween::Update(float deltaTime)
{
    if (_paused || _completed || _killed) return;
    _elapsed += max(0.f, deltaTime) * _timeScale;
    if (_elapsed < _delay) return;

    InitializeIfNeeded();
    if (!_started)
    {
        _started = true;
        if (_onStart) _onStart();
    }

    const float activeTime = _elapsed - _delay;
    if (_duration <= 0.f)
    {
        Complete();
        return;
    }
    if (_loops >= 0 && activeTime >= _duration * static_cast<float>(_loops + 1))
    {
        ApplyFinalValue();
        _completed = true;
        if (_onComplete) _onComplete();
        return;
    }

    const int cycle = static_cast<int>(activeTime / _duration);
    float progress = fmodf(activeTime, _duration) / _duration;
    if (_loopType == LoopType::Yoyo && (cycle % 2 == 1)) progress = 1.f - progress;
    if (_applicator) _applicator(EvaluateEase(_ease, progress));
    if (_onUpdate) _onUpdate(progress);
}

float Tween::EvaluateEase(EaseType ease, float value)
{
    value = clamp(value, 0.f, 1.f);
    switch (ease)
    {
    case EaseType::InSine: return 1.f - cosf(value * Pi * 0.5f);
    case EaseType::OutSine: return sinf(value * Pi * 0.5f);
    case EaseType::InOutSine: return -(cosf(Pi * value) - 1.f) * 0.5f;
    case EaseType::InQuad: return value * value;
    case EaseType::OutQuad: return 1.f - (1.f - value) * (1.f - value);
    case EaseType::InOutQuad: return value < 0.5f ? 2.f * value * value : 1.f - powf(-2.f * value + 2.f, 2.f) * 0.5f;
    case EaseType::InCubic: return value * value * value;
    case EaseType::OutCubic: return 1.f - powf(1.f - value, 3.f);
    case EaseType::InOutCubic: return value < 0.5f ? 4.f * value * value * value : 1.f - powf(-2.f * value + 2.f, 3.f) * 0.5f;
    case EaseType::InBack:
    {
        constexpr float c1 = 1.70158f;
        return (c1 + 1.f) * value * value * value - c1 * value * value;
    }
    case EaseType::OutBack:
    {
        constexpr float c1 = 1.70158f;
        return 1.f + (c1 + 1.f) * powf(value - 1.f, 3.f) + c1 * powf(value - 1.f, 2.f);
    }
    case EaseType::InOutBack:
    {
        constexpr float c2 = 1.70158f * 1.525f;
        return value < 0.5f
            ? powf(2.f * value, 2.f) * ((c2 + 1.f) * 2.f * value - c2) * 0.5f
            : (powf(2.f * value - 2.f, 2.f) * ((c2 + 1.f) * (value * 2.f - 2.f) + c2) + 2.f) * 0.5f;
    }
    case EaseType::InBounce: return 1.f - OutBounce(1.f - value);
    case EaseType::OutBounce: return OutBounce(value);
    case EaseType::InOutBounce: return value < 0.5f ? (1.f - OutBounce(1.f - 2.f * value)) * 0.5f : (1.f + OutBounce(2.f * value - 1.f)) * 0.5f;
    default: return value;
    }
}

void TweenManager::Add(const TweenPtr& tween)
{
    (_isUpdating ? _pendingTweens : _tweens).push_back(tween);
}

void TweenManager::Update(float deltaTime)
{
    _isUpdating = true;
    for (const TweenPtr& tween : _tweens) tween->Update(deltaTime);
    _isUpdating = false;
    _tweens.erase(remove_if(_tweens.begin(), _tweens.end(), [](const TweenPtr& tween)
    {
        return tween->IsComplete() || tween->IsKilled();
    }), _tweens.end());
    if (!_pendingTweens.empty())
    {
        _tweens.insert(_tweens.end(), _pendingTweens.begin(), _pendingTweens.end());
        _pendingTweens.clear();
    }
}

void TweenManager::Clear(bool invokeKillCallbacks)
{
    if (invokeKillCallbacks)
    {
        for (const TweenPtr& tween : _tweens) tween->Kill();
        for (const TweenPtr& tween : _pendingTweens) tween->Kill();
    }
    _tweens.clear();
    _pendingTweens.clear();
}

TweenPtr TweenManager::DOMove(Transform* transform, const Vec3& endValue, float duration)
{
    TransformRef target(transform);
    TweenPtr tween = To<Vec3>(
        [target]() { Transform* value = target.Resolve(); return value ? value->GetPosition() : Vec3::Zero; },
        [target](const Vec3& value) { Transform* resolved = target.Resolve(); if (resolved) resolved->SetPosition(value); },
        endValue, duration);
    tween->SetTarget(transform);
    return tween;
}

TweenPtr TweenManager::DOLocalMove(Transform* transform, const Vec3& endValue, float duration)
{
    TransformRef target(transform);
    TweenPtr tween = To<Vec3>(
        [target]() { Transform* value = target.Resolve(); return value ? value->GetLocalPosition() : Vec3::Zero; },
        [target](const Vec3& value) { Transform* resolved = target.Resolve(); if (resolved) resolved->SetLocalPosition(value); },
        endValue, duration);
    tween->SetTarget(transform);
    return tween;
}

TweenPtr TweenManager::DORotate(Transform* transform, const Vec3& endValue, float duration)
{
    TransformRef target(transform);
    TweenPtr tween = To<Vec3>(
        [target]() { Transform* value = target.Resolve(); return value ? value->GetRotation() : Vec3::Zero; },
        [target](const Vec3& value) { Transform* resolved = target.Resolve(); if (resolved) resolved->SetRotation(value); },
        endValue, duration);
    tween->SetTarget(transform);
    return tween;
}

TweenPtr TweenManager::DOLocalRotate(Transform* transform, const Vec3& endValue, float duration)
{
    TransformRef target(transform);
    TweenPtr tween = To<Vec3>(
        [target]() { Transform* value = target.Resolve(); return value ? value->GetLocalRotation() : Vec3::Zero; },
        [target](const Vec3& value) { Transform* resolved = target.Resolve(); if (resolved) resolved->SetLocalRotation(value); },
        endValue, duration);
    tween->SetTarget(transform);
    return tween;
}

TweenPtr TweenManager::DOPunchLocalRotate(Transform* transform, const Vec3& punch, float duration, int vibrato, float elasticity)
{
    TransformRef target(transform);
    auto startRotation = make_shared<Vec3>();
    const int clampedVibrato = max(1, vibrato);
    const float clampedElasticity = clamp(elasticity, 0.f, 1.f);
    TweenPtr tween(new Tween(duration,
        [target, startRotation]()
        {
            Transform* value = target.Resolve();
            *startRotation = value ? value->GetLocalRotation() : Vec3::Zero;
        },
        [target, startRotation, punch, clampedVibrato, clampedElasticity](float t)
        {
            Transform* resolved = target.Resolve();
            if (!resolved) return;

            const float decay = powf(1.f - t, 1.f + (1.f - clampedElasticity) * 2.f);
            const float oscillation = sinf(t * Pi * 2.f * static_cast<float>(clampedVibrato));
            resolved->SetLocalRotation(*startRotation + punch * (oscillation * decay));
        }));
    tween->SetTarget(transform);
    Add(tween);
    return tween;
}

TweenPtr TweenManager::DOScale(Transform* transform, const Vec3& endValue, float duration)
{
    TransformRef target(transform);
    TweenPtr tween = To<Vec3>(
        [target]() { Transform* value = target.Resolve(); return value ? value->GetLocalScale() : Vec3::One; },
        [target](const Vec3& value) { Transform* resolved = target.Resolve(); if (resolved) resolved->SetLocalScale(value); },
        endValue, duration);
    tween->SetTarget(transform);
    return tween;
}

TweenPtr TweenManager::DOColor(UIImage* image, const Color& endValue, float duration)
{
    ComponentRef<UIImage> target(image);
    TweenPtr tween = To<Color>(
        [target]()
        {
            UIImage* value = target.Resolve();
            return value ? value->GetColor() : Colors::White;
        },
        [target](const Color& value)
        {
            UIImage* resolved = target.Resolve();
            if (resolved) resolved->SetColor(value);
        },
        endValue, duration);
    tween->SetTarget(image);
    return tween;
}

void TweenManager::PauseAll()
{
    for (const TweenPtr& tween : _tweens) tween->Pause();
    for (const TweenPtr& tween : _pendingTweens) tween->Pause();
}

void TweenManager::PlayAll()
{
    for (const TweenPtr& tween : _tweens) tween->Play();
    for (const TweenPtr& tween : _pendingTweens) tween->Play();
}

void TweenManager::KillAll(bool complete)
{
    for (const TweenPtr& tween : _tweens) tween->Kill(complete);
    for (const TweenPtr& tween : _pendingTweens) tween->Kill(complete);
}

void TweenManager::Pause(const void* target)
{
    for (const TweenPtr& tween : _tweens) if (tween->GetTarget() == target) tween->Pause();
    for (const TweenPtr& tween : _pendingTweens) if (tween->GetTarget() == target) tween->Pause();
}

void TweenManager::Play(const void* target)
{
    for (const TweenPtr& tween : _tweens) if (tween->GetTarget() == target) tween->Play();
    for (const TweenPtr& tween : _pendingTweens) if (tween->GetTarget() == target) tween->Play();
}

void TweenManager::Kill(const void* target, bool complete)
{
    for (const TweenPtr& tween : _tweens) if (tween->GetTarget() == target) tween->Kill(complete);
    for (const TweenPtr& tween : _pendingTweens) if (tween->GetTarget() == target) tween->Kill(complete);
}

size_t TweenManager::GetActiveCount() const
{
    const auto isActive = [](const TweenPtr& tween)
    {
        return !tween->IsComplete() && !tween->IsKilled();
    };
    return count_if(_tweens.begin(), _tweens.end(), isActive)
        + count_if(_pendingTweens.begin(), _pendingTweens.end(), isActive);
}
