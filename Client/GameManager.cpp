#include "pch.h"
#include "GameManager.h"
#include "OnGUIUtils.h"
#include "Player.h"
#include "Zombie.h"
#include "TargetFollower.h"
#include "UIImage.h"
#include "Button.h"
#include "Camera.h"
#include "ThirdPersonCamMove.h"

void GameManager::Awake()
{
    ASSERT(_instance == nullptr || _instance == this, "GameManager already exists");
    if (_instance != nullptr && _instance != this)
        return;
    _instance = this;

    if (_playerPrefab.Resolve() == nullptr)
        _playerPrefab = CUR_SCENE->FindComponent<Player>();
    if (_zombiePrefab.Resolve() == nullptr)
        _zombiePrefab = CUR_SCENE->FindComponent<Zombie>();

    if (_playerSpawnPoint.Resolve() == nullptr)
        _playerSpawnPoint = CUR_SCENE->Add("PlayerSpawnPoint", GetTransform()).Resolve()->GetTransform();
   
    if (_titlePoint.Resolve() == nullptr)
        _titlePoint = CUR_SCENE->Add("TitlePoint", GetTransform()).Resolve()->GetTransform();

    _titlePoint.Resolve()->SetPosition(Vec3(- 5.9, 9.3, 85.4));
    _titlePoint.Resolve()->SetRotation(Vec3(-14.7, 9.5, 0));

    TargetFollower* cameraFollower = _cameraFollower.Resolve();
    if (_cameraFollower.Resolve() != nullptr)
    {
        cameraFollower->SetTarget(_titlePoint.Resolve());
        cameraFollower->UpdateImmediateFollow();
    }

    UIImage* titleImage = _titleImage.Resolve();
    if (titleImage != nullptr)
    {
        Button* button = titleImage->GetGameObject()->GetComponentInChildren<Button>();
        button->AddOnClickedEvent(
            [&](){
                _titleImage.Resolve()->GetGameObject()->SetActive(false);

                Player* player = GameObject::Instantiate(_playerPrefab.Resolve());
                _myPlayer = player;

                TargetFollower* cameraFollower = _cameraFollower.Resolve();
                cameraFollower->SetTarget(player->GetTransform());
                cameraFollower->SetPositionInterpolationSpeed(4.f);
                cameraFollower->SetEnabled(true);
                cameraFollower->GetGameObject()->GetComponentInChildren<Camera>()->GetGameObject()->AddComponent<ThirdPersonCamMove>();
                //{
                //    unique_ptr<ThirdPersonCamMove> thirdPersonCamMove = make_unique<ThirdPersonCamMove>();
                //    thirdPersonCamMove->SetTarget(obj->GetTransformRef());

        });
    }
}

void GameManager::Update()
{
    UpdateGameState();

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

bool GameManager::OnGUI()
{
    bool changed = false;

    changed |= OnGUIUtils::DrawComponentRef("PlayerPrefab", _playerPrefab);
    changed |= OnGUIUtils::DrawComponentRef("ZombiePrefab", _zombiePrefab);
    changed |= OnGUIUtils::DrawComponentRef("TitlePoint", _titlePoint);
    changed |= OnGUIUtils::DrawComponentRef("PlayerSpawnPoint", _playerSpawnPoint);

    return changed;
}

void GameManager::OnCreateMyPlayer(Protocol::Player myPlayer)
{
    if (_myPlayer.Resolve() != nullptr)
    {
        DBG->LogError("My player already exists.");
        return;
    }

    Player* player = GameObject::Instantiate(_playerPrefab.Resolve(), nullptr);
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

    Player* player = GameObject::Instantiate(_playerPrefab.Resolve(), nullptr);
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

void GameManager::SetCameraFollower(ComponentRef<TargetFollower> cameraFollower)
{
    _cameraFollower = cameraFollower;
    TargetFollower* cameraFollowerPtr = _cameraFollower.Resolve();
    cameraFollowerPtr->SetTarget(_titlePoint.Resolve());
    cameraFollowerPtr->UpdateImmediateFollow();
}

void GameManager::PushJob(function<void(void)> func)
{
    WRITE_LOCK;
    _jobQueue.push_back(func);
}

void GameManager::UpdateGameState()
{
    switch (_gameState)
    {
    case GameState::Title:
    case GameState::Login:
    case GameState::World:
    }
}
