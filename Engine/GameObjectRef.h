#pragma once
#include "GuidRef.h"

class GameObjectRef : public GuidRef
{
public:
    GameObject* Resolve() const;
};

using GameObjectRefSet = std::unordered_set<GameObjectRef, GuidRefHash, GuidRefEq>;