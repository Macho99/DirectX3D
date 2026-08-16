#pragma once
using ComponentFactory = function<unique_ptr<class Component>()>;
using ComponentMatcher = function<bool(class Component*)>;
using ComponentSerializedCopier = function<void(class Component*, class Component*)>;
enum class ComponentType : uint8;

template<class T>
bool MatchComponentType(Component* component);

template<class T>
void CopyComponentBySerialization(Component* source, Component* target);

struct ComponentDesc
{
    ComponentType type;
    const char* name;
    ComponentFactory factory;
    ComponentMatcher matcher;
    ComponentSerializedCopier serializedCopier;
};

class ComponentRegistry
{
public:
    static ComponentRegistry& Get()
    {
        static ComponentRegistry instance;
        return instance;
    }

    void Register(ComponentType type, const char* name, ComponentFactory factory,
        ComponentMatcher matcher, ComponentSerializedCopier serializedCopier)
    {
        _descs.push_back({ type, name, factory, matcher, serializedCopier });
    }

    void Init();
    const vector<ComponentDesc>& GetDescs() const { return _descs; }

private:
    vector<ComponentDesc> _descs;
};

#define DECLARE_COMPONENT(TYPE)                                      \
public:                                                              \
    static constexpr ComponentType StaticType = ComponentType::TYPE; \
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
                StaticType,                                         \
                StaticName(),                                       \
                &TYPE::CreateInstance,                              \
                &MatchComponentType<TYPE>,                          \
                &CopyComponentBySerialization<TYPE>);               \
        }                                                           \
    };                                                              \
    inline static AutoRegister _autoRegister;
