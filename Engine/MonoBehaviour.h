#pragma once
#include "Component.h"

class MonoBehaviour : public Component
{
	using Super = Component;

public:
	MonoBehaviour();
	~MonoBehaviour();

	virtual void Awake() override;
	virtual void Update() override;

};

#define DECLARE_MONO_BEHAVIOUR(TYPE)                                 \
public:                                                              \
    static const char* StaticName() { return #TYPE; }                \
    static std::unique_ptr<Component> CreateInstance()              \
    { return std::make_unique<TYPE>(); }                            \
    static void EnsureAutoRegister() { (void)_autoRegister; }       \
private:                                                             \
    struct AutoRegister                                             \
    {                                                               \
        AutoRegister()                                              \
        {                                                           \
            ComponentRegistry::Get().Register(                      \
                ComponentType::Script,                              \
                StaticName(),                                       \
                &TYPE::CreateInstance);                             \
        }                                                           \
    };                                                              \
    inline static AutoRegister _autoRegister;