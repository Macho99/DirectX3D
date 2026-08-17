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
    if (_myPlayer.Resolve() != nullptr)
    {
        DBG->LogError("My player already exists.");
        return;
    }

    GameObject* playerObj = GameObject::Instantiate(_playerPrefab.Resolve()->GetGameObjectRef(), nullptr).Resolve();
    Player* player = playerObj->GetComponent<Player>();
    player->Init(myPlayer.id(), myPlayer.name(), true);

    _myPlayer = player;
}

void GameManager::OnOtherPlayerEnter(Protocol::Player otherPlayer)
{
    if (_players.find(otherPlayer.id()) != _players.end())
    {
        DBG->LogError("Player ID %llu already exists in _players map.", otherPlayer.id());
        return;
    }

    GameObject* playerObj = GameObject::Instantiate(_playerPrefab.Resolve()->GetGameObjectRef(), nullptr).Resolve();
    Player* player = playerObj->GetComponent<Player>();
    player->Init(otherPlayer.id(), otherPlayer.name(), false);

    _players[otherPlayer.id()] = player;
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
    
    CUR_SCENE->Remove(_myPlayer.Resolve()->GetGameObjectRef());
    _myPlayer = ComponentRef<Player>();
}

void GameManager::PushJob(function<void(void)> func)
{
    WRITE_LOCK;
    _jobQueue.push_back(func);
}
