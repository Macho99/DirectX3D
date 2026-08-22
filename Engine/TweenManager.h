#pragma once

#include <type_traits>

class Transform;
class UIImage;

enum class EaseType : uint8
{
    Linear,
    InSine, OutSine, InOutSine,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InBack, OutBack, InOutBack,
    InBounce, OutBounce, InOutBounce
};

enum class LoopType : uint8
{
    Restart,
    Yoyo
};

class Tween
{
public:
    using Callback = function<void()>;
    using UpdateCallback = function<void(float)>;

    Tween* SetEase(EaseType ease);
    Tween* SetDelay(float delay);
    // loops is the repeat count after the first play. -1 repeats forever.
    Tween* SetLoops(int loops, LoopType loopType = LoopType::Restart);
    Tween* SetTimeScale(float timeScale);
    Tween* SetTarget(const void* target);
    Tween* OnStart(Callback callback);
    Tween* OnUpdate(UpdateCallback callback);
    Tween* OnComplete(Callback callback);
    Tween* OnKill(Callback callback);
    Tween* Pause();
    Tween* Play();
    Tween* Restart();
    Tween* Complete();
    Tween* Kill(bool complete = false);

    bool IsPlaying() const { return !_paused && !_completed && !_killed; }
    bool IsComplete() const { return _completed; }
    bool IsKilled() const { return _killed; }
    float GetElapsedPercentage() const;
    const void* GetTarget() const { return _target; }

private:
    friend class TweenManager;
    using Initializer = function<void()>;
    using Applicator = function<void(float)>;

    Tween(float duration, Initializer initializer, Applicator applicator);
    void Update(float deltaTime);
    void ApplyFinalValue();
    void InitializeIfNeeded();
    static float EvaluateEase(EaseType ease, float value);

    float _duration = 0.f;
    float _delay = 0.f;
    float _elapsed = 0.f;
    float _timeScale = 1.f;
    int _loops = 0;
    LoopType _loopType = LoopType::Restart;
    EaseType _ease = EaseType::OutQuad;
    bool _initialized = false;
    bool _started = false;
    bool _paused = false;
    bool _completed = false;
    bool _killed = false;
    const void* _target = nullptr;
    Initializer _initializer;
    Applicator _applicator;
    Callback _onStart;
    UpdateCallback _onUpdate;
    Callback _onComplete;
    Callback _onKill;
};

using TweenPtr = shared_ptr<Tween>;

namespace TweenDetail
{
    template<class T, class Enable = void>
    struct ValueLerp;

    template<class T>
    struct ValueLerp<T, enable_if_t<is_arithmetic_v<T>>>
    {
        static T Apply(const T& start, const T& end, float t)
        {
            return static_cast<T>(start + (end - start) * t);
        }
    };

    template<> struct ValueLerp<Vec2>
    {
        static Vec2 Apply(const Vec2& start, const Vec2& end, float t) { return Vec2::Lerp(start, end, t); }
    };
    template<> struct ValueLerp<Vec3>
    {
        static Vec3 Apply(const Vec3& start, const Vec3& end, float t) { return Vec3::Lerp(start, end, t); }
    };
    template<> struct ValueLerp<Vec4>
    {
        static Vec4 Apply(const Vec4& start, const Vec4& end, float t) { return Vec4::Lerp(start, end, t); }
    };
    template<> struct ValueLerp<Color>
    {
        static Color Apply(const Color& start, const Color& end, float t)
        {
            return Color(start.x + (end.x - start.x) * t,
                start.y + (end.y - start.y) * t,
                start.z + (end.z - start.z) * t,
                start.w + (end.w - start.w) * t);
        }
    };
    template<> struct ValueLerp<Quaternion>
    {
        static Quaternion Apply(const Quaternion& start, const Quaternion& end, float t)
        {
            return Quaternion::Slerp(start, end, t);
        }
    };
}

class TweenManager
{
    DECLARE_SINGLE(TweenManager);

public:
    void Update(float deltaTime);
    void Clear(bool invokeKillCallbacks = false);

    template<class T, class Getter, class Setter>
    TweenPtr To(Getter&& getter, Setter&& setter, const T& endValue, float duration)
    {
        using Value = decay_t<T>;
        auto startValue = make_shared<Value>();
        auto getterFunction = function<Value()>(forward<Getter>(getter));
        auto setterFunction = function<void(const Value&)>(forward<Setter>(setter));
        auto tween = TweenPtr(new Tween(duration,
            [startValue, getterFunction]() { *startValue = getterFunction(); },
            [startValue, endValue, setterFunction](float t)
            {
                setterFunction(TweenDetail::ValueLerp<Value>::Apply(*startValue, endValue, t));
            }));
        Add(tween);
        return tween;
    }

    template<class T>
    TweenPtr To(T& value, const T& endValue, float duration)
    {
        TweenPtr tween = To<T>(
            [&value]() { return value; },
            [&value](const T& newValue) { value = newValue; },
            endValue, duration);
        tween->SetTarget(&value);
        return tween;
    }

    TweenPtr DOMove(Transform* transform, const Vec3& endValue, float duration);
    TweenPtr DOLocalMove(Transform* transform, const Vec3& endValue, float duration);
    TweenPtr DORotate(Transform* transform, const Vec3& endValue, float duration);
    TweenPtr DOLocalRotate(Transform* transform, const Vec3& endValue, float duration);
    TweenPtr DOPunchLocalRotate(Transform* transform, const Vec3& punch, float duration, int vibrato = 10, float elasticity = 1.f);
    TweenPtr DOScale(Transform* transform, const Vec3& endValue, float duration);
    TweenPtr DOColor(UIImage* image, const Color& endValue, float duration);

    void PauseAll();
    void PlayAll();
    void KillAll(bool complete = false);
    void Pause(const void* target);
    void Play(const void* target);
    void Kill(const void* target, bool complete = false);
    size_t GetActiveCount() const;

private:
    void Add(const TweenPtr& tween);
    vector<TweenPtr> _tweens;
    vector<TweenPtr> _pendingTweens;
    bool _isUpdating = false;
};
