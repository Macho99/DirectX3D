#include "pch.h"
#include "GameManager.h"
#include "Player.h"

void GameManager::Awake()
{
}

void GameManager::Update()
{
    vector<function<void(void)>> tempQueue;
    {
        WRITE_LOCK;
        tempQueue = std::move(_jobQueue);
    }

    for (int i = 0; i < tempQueue.size(); i++)
    {
        tempQueue[i]();
    }
}

void GameManager::OnCreateMyPlayer(Protocol::Player myPlayer)
{
    GameObject* playerObj = CUR_SCENE->Add(myPlayer.name()).Resolve();
    playerObj->AddComponent(make_unique<Player>());
    ComponentRef<Player> playerRef = playerObj->GetScriptComponent<Player>();
    playerRef.Resolve()->Init(myPlayer.id(), myPlayer.name(), true);

    _players[myPlayer.id()] = playerRef;
}

void GameManager::OnOtherPlayerEnter(Protocol::Player otherPlayer)
{
    GameObject* playerObj = CUR_SCENE->Add(otherPlayer.name()).Resolve();
    playerObj->AddComponent(make_unique<Player>());
    ComponentRef<Player> playerRef = playerObj->GetScriptComponent<Player>();
    playerRef.Resolve()->Init(otherPlayer.id(), otherPlayer.name(), false);

    _players[otherPlayer.id()] = playerRef;
}

void GameManager::OnOtherPlayerExit(uint64 otherPlayerId)
{
    _players.erase(otherPlayerId);
}

void GameManager::PushJob(function<void(void)> func)
{
    WRITE_LOCK;
    _jobQueue.push_back(func);
}
