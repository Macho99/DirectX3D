#include "pch.h"
#include "Player.h"

void Player::Init(uint64 playerId, string name, bool isMyPlayer)
{
    _playerId = playerId;
    _name = name;
    _isMyPlayer = isMyPlayer;
}
