#pragma once
#include <MonoBehaviour.h>
class Player : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(Player);
public:
    void Init(uint64 playerId, string name, bool isMyPlayer);

private:
    uint64 _playerId;
    string _name;
    bool _isMyPlayer;
};

