#pragma once
#include "GuidRef.h"

struct GameObjectRef : public GuidRef
{
public:
    GameObjectRef() : GuidRef() {}
private:
    GameObjectRef(Guid guid) : GuidRef(guid) {}
    GameObjectRef(const GuidRef& other) : GuidRef(other) {}

public:
    GameObject* Resolve() const;

    friend class Scene;
    friend class GameObject;
};

using GameObjectRefSet = std::unordered_set<GameObjectRef, GuidRefHash, GuidRefEq>;