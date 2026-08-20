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
#include "ModelAnimator.h"
#include "Model.h"
#include "InputText.h"
#include "MyBillboard.h"

namespace
{
    Vec3 ToVec3(const Protocol::Vec3& protoVec)
    {
        return Vec3(protoVec.x(), protoVec.y(), protoVec.z());
    }

    Vec2 ToVec2(const Protocol::Vec2& protoVec)
    {
        return Vec2(protoVec.x(), protoVec.y());
    }

    void UpdateServerTransform(Character* character, const Protocol::TransformData& transformData)
    {
        character->UpdateServerTransform(
            ToVec3(transformData.pos()), 
            transformData.yaw(), 
            ToVec2(transformData.velocity()));
    }
}

void GameManager::Start()
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
        Transform* camTrans = cameraFollower->GetGameObject()->GetComponentInChildren<Camera>()->GetTransform();
        camTrans->SetLocalRotation(Vec3(0, 0, 0));
        camTrans->SetLocalPosition(Vec3(0, 0, -4));
    }

    UIImage* titleImage = _titleImage.Resolve();
    if (titleImage != nullptr && _cameraSpawnPoint.Resolve() != nullptr)
    {
        Button* button = titleImage->GetGameObject()->GetComponentInChildren<Button>();
        button->AddOnClickedEvent(
            [&](){
                SERVER_CONNECT->TryConnect();
        });
    }

    Player* player = _playerPrefab.Resolve();
    if (player != nullptr)
    {
        Model* model = player->GetGameObject()->GetComponent<ModelAnimator>()->GetModel().Resolve();
        if (model != nullptr)
        {
            model->EnsureAnimationTexture();
        }
        else
        {
            DBG->LogError("Player model is null.");
        }
    }
    else
    {
        DBG->LogError("Player prefab is null.");
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
    if (SERVER_CONNECT != nullptr)
    {
        SERVER_CONNECT->OnDestroy();
    }

    if (_instance != this)
        return;
    _instance = nullptr;
}

bool GameManager::OnGUI()
{
    bool changed = false;

    changed |= OnGUIUtils::DrawComponentRef("CameraFollower", _cameraFollower);
    changed |= OnGUIUtils::DrawComponentRef("PlayerPrefab", _playerPrefab);
    changed |= OnGUIUtils::DrawComponentRef("ZombiePrefab", _zombiePrefab);
    changed |= OnGUIUtils::DrawComponentRef("TitlePoint", _titlePoint);
    changed |= OnGUIUtils::DrawComponentRef("PlayerSpawnPoint", _playerSpawnPoint);
    changed |= OnGUIUtils::DrawComponentRef("TitleImage", _titleImage);
    changed |= OnGUIUtils::DrawComponentRef("LoginButton", _loginButton);
    changed |= OnGUIUtils::DrawComponentRef("CameraSpawnPoint", _cameraSpawnPoint);
    changed |= OnGUIUtils::DrawComponentRef("UserNameInput", _usernameInput);

    return changed;
}

void GameManager::OnCreateMyPlayer(Protocol::Player myPlayer)
{
    Player* player = _myPlayer.Resolve();
    player->Init(myPlayer.id(), myPlayer.name(), true);

    TargetFollower* targetFollower = _cameraFollower.Resolve();
    targetFollower->SetTarget(player->GetTransform());
    targetFollower->SetFollowRotation(false, false, false);
    targetFollower->SetEnabled(true);
    ThirdPersonCamMove* thirdPersonCamMove = targetFollower->GetGameObject()->GetComponentInChildren<ThirdPersonCamMove>();
    thirdPersonCamMove->SetTarget(player->GetTransform());

    _loginButton.Resolve()->GetTransform()->GetParent()->GetGameObject()->SetActive(false);
    _players[myPlayer.id()] = player;

    SetGameState(GameState::World);
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
    player->GetGameObject()->SetActive(true);
    UpdateServerTransform(player, otherPlayer.transform());

    {
        GameObject* nameTextObj = CUR_SCENE->Add("NameText", player->GetTransform()).Resolve();
        Transform* nameTransform = nameTextObj->GetTransform();
        nameTransform->SetLocalPosition(Vec3(0, 210, 0));
        nameTransform->SetLocalScale(Vec3(100));
        nameTextObj->AddComponent<MyBillboard>();
        Text* nameText = nameTextObj->AddComponent<Text>();
        nameText->SetFontSize(0.4f);
        nameText->SetHorizontalStart(TextHorizontalStart::Center);
        nameText->SetIgnoreMouseInput(true);
        nameText->SetText(otherPlayer.name());
    }

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

void GameManager::OnMovePlayer(Protocol::TransformData playerMove)
{
    auto it = _players.find(playerMove.id());
    if (it == _players.end())
    {
        DBG->LogError("Player ID %llu not found in _players map.", playerMove.id());
        return;
    }

    Player* player = it->second.Resolve();
    UpdateServerTransform(player, playerMove);
}

void GameManager::OnMoveMonster(Protocol::TransformData monsterMove)
{
    auto it = _monsters.find(monsterMove.id());
    if (it == _monsters.end())
    {
        DBG->LogError("Monster ID %llu not found in _monsters map.", monsterMove.id());
        return;
    }

    Zombie* monster = it->second.Resolve();
    UpdateServerTransform(monster, monsterMove);
}

void GameManager::OnDisconnect()
{
    for (auto& pair : _players)
    {
        CUR_SCENE->Remove(pair.second.Resolve()->GetGameObjectRef());
    }
    _players.clear();
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
    case GameState::Login:
    case GameState::World:
        break;
    }
}

void GameManager::SetGameState(GameState gameState)
{
    DBG->Log("GameState Change: %d -> %d", (int)_gameState , (int)gameState);
    _gameState = gameState;
}

void GameManager::LoginState()
{
    Button* loginButton = _loginButton.Resolve();
    InputText* usernameInput = _usernameInput.Resolve();

    if (loginButton == nullptr || usernameInput == nullptr)
    {
        DBG->LogError("Login button or username input is null.");
    }

    loginButton->AddOnClickedEvent([&]()
        {
            string username = _usernameInput.Resolve()->GetText();
            if (username.empty())
            {
                DBG->LogError("Username is empty.");
                return;
            }
            Protocol::C_LOGIN loginPacket;
            loginPacket.set_name(username);
            auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPacket);
            SERVER_CONNECT->SendPacket(sendBuffer);
        });


    loginButton->GetTransform()->GetParent()->GetGameObject()->SetActive(true);

    SetGameState(GameState::Login);
}

void GameManager::ConnectedState()
{
    Transform* playerSpawnPoint = _playerSpawnPoint.Resolve();

    Player* player = GameObject::Instantiate(_playerPrefab.Resolve());
    player->GetGameObject()->SetActive(true);
    player->GetTransform()->SetWorldMatrix(_playerSpawnPoint.Resolve()->GetWorldMatrix());
    player->GetTransform()->SetScale(_playerPrefab.Resolve()->GetTransform()->GetScale());
    player->UpdateServerTransform(playerSpawnPoint->GetPosition(), playerSpawnPoint->GetRotation().y, Vec2::Zero);
    _myPlayer = player;

    const float duration = 3.f;
    TWEEN->DORotate(_cameraFollower.Resolve()->GetTransform(), _playerSpawnPoint.Resolve()->GetRotation(), duration);
    TweenPtr tween = TWEEN->DOMove(_cameraFollower.Resolve()->GetTransform(), _cameraSpawnPoint.Resolve()->GetPosition(), duration);
    tween->OnComplete([&]()
        {
            LoginState();
        });

    _titleImage.Resolve()->GetTransform()->GetParent()->GetGameObject()->SetActive(false);

    SetGameState(GameState::Connected);
}
