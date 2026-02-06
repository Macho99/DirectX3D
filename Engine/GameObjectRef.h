#pragma once
#include "GuidRef.h"

struct GameObjectRef : public GuidRef
{
public:
    GameObjectRef() : GuidRef() {}
    GameObjectRef(Guid guid) : GuidRef(guid) {}
    GameObjectRef(const GuidRef& other) : GuidRef(other) {}

    GameObject* Resolve() const;
};

using GameObjectRefSet = std::unordered_set<GameObjectRef, GuidRefHash, GuidRefEq>;