#pragma once
#include <MonoBehaviour.h>

class Player;
class Zombie;
class TargetFollower;

enum class GameState
{
    Title,
    TitleToLogin,
    Login,
    World,

    End,
};

class GameManager : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(GameManager)
public:
    static GameManager* GetInstance() { return _instance; }

    virtual void Awake();
    virtual void Update();
    virtual void OnDestroy();
    virtual bool OnGUI() override;

    void OnCreateMyPlayer(Protocol::Player myPlayer);
    void OnOtherPlayerEnter(Protocol::Player otherPlayer);
    void OnOtherPlayerExit(uint64 otherPlayerId);
    void OnDisconnect();
    void SetCameraFollower(ComponentRef<TargetFollower> cameraFollower);
    virtual int GetVersion() const override { return 2; }

    void PushJob(function<void(void)> func);
    void UpdateGameState();
    GameState GetGameState() const { return _gameState; }
    void SetGameState(GameState gameState);

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_playerPrefab));
        ar(CEREAL_NVP(_zombiePrefab));

        if (_version >= 1)
        {
            ar(CEREAL_NVP(_titlePoint));
            ar(CEREAL_NVP(_playerSpawnPoint));
        }

        if (_version >= 2)
        {
            ar(CEREAL_NVP(_titleImage));
            ar(CEREAL_NVP(_loginButton));
        }

        if (_version >= 3)
        {
            ar(CEREAL_NVP(_cameraFollower));
        }
    }

private:
    inline static GameManager* _instance = nullptr;

    USE_LOCK
    vector<function<void(void)>> _jobQueue;

    ComponentRef<Player> _playerPrefab;
    unordered_map<uint64, ComponentRef<Player>> _players;
    ComponentRef<Player> _myPlayer;

    ComponentRef<Zombie> _zombiePrefab;
    ComponentRef<TargetFollower> _cameraFollower;

    ComponentRef<Transform> _titlePoint;
    ComponentRef<Transform> _playerSpawnPoint;

    ComponentRef<UIImage> _titleImage;
    ComponentRef<Button> _loginButton;
    GameState _gameState;
};

#define GM GameManager::GetInstance()
