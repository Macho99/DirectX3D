#pragma once
using ComponentFactory = function<unique_ptr<class Component>()>;
enum class ComponentType : uint8;

struct ComponentDesc
{
    ComponentType type;
    const char* name;
    ComponentFactory factory;
};

class ComponentRegistry
{
public:
    static ComponentRegistry& Get()
    {
        static ComponentRegistry instance;
        return instance;
    }

    void Register(ComponentType type, const char* name, ComponentFactory factory)
    {
        _descs.push_back({ type, name, factory });
    }

    const std::vector<ComponentDesc>& GetDescs() const { return _descs; }

private:
    std::vector<ComponentDesc> _descs;
};

#define DECLARE_COMPONENT(TYPE)                                      \
public:                                                              \
    static constexpr ComponentType StaticType = ComponentType::TYPE; \
    static const char* StaticName() { return #TYPE; }                \
    static std::unique_ptr<Component> CreateInstance()              \
    { return std::make_unique<TYPE>(); }                            \
private:                                                             \
    struct AutoRegister                                             \
    {                                                               \
        AutoRegister()                                              \
        {                                                           \
            ComponentRegistry::Get().Register(                      \
                StaticType,                                         \
                StaticName(),                                       \
                &TYPE::CreateInstance);                             \
        }                                                           \
    };                                                              \
    inline static AutoRegister _autoRegister;