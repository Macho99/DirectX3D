#include "pch.h"
#include "GameManager.h"
#include "Player.h"

void GameManager::Awake()
{
    ASSERT(_instance == nullptr || _instance == this, "GameManager already exists");
    if (_instance != nullptr && _instance != this)
        return;
    _instance = this;

    _playerPrefab = CUR_SCENE->FindComponent<Player>();
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

void GameManager::OnDestroy()
{
    if (_instance != this)
        return;
    _instance = nullptr;
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
    auto it = _players.find(otherPlayerId);
    if (it == _players.end())
    {
        DBG->LogError("Player ID %llu not found in _players map.", otherPlayerId);
        return;
    }

    CUR_SCENE->Remove(it->second.Resolve()->GetGameObjectRef());

    _players.erase(it);
}

void GameManager::OnDisconnect()
{
    for (auto& pair : _players)
    {
        CUR_SCENE->Remove(pair.second.Resolve()->GetGameObjectRef());
    }
    _players.clear();
}

void GameManager::PushJob(function<void(void)> func)
{
    WRITE_LOCK;
    _jobQueue.push_back(func);
}
