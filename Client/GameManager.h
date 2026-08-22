#pragma once
#include <MonoBehaviour.h>

class Player;
class Zombie;
class TargetFollower;
class InputText;
class ParticleSystem;
class Text;

enum class GameState
{
    Title = 0,
    Connected,
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

    virtual void Start();
    virtual void Update();
    virtual void OnDestroy();
    virtual bool OnGUI() override;

    void OnCreateMyPlayer(Protocol::Player myPlayer);
    void OnOtherPlayerEnter(Protocol::Player otherPlayer);
    void OnOtherPlayerExit(uint64 otherPlayerId);
    void OnMovePlayer(Protocol::TransformData playerMove);
    void OnPlayerAnimation(Protocol::AnimationData animationData);
    void OnPlayerHpChange(Protocol::HealthData healthData);
    void OnMyPlayerStatChange(Protocol::StatData statData);

    void OnMoveMonster(Protocol::TransformData monsterMove);
    void OnMonsterSpawn(Protocol::Monster monster);
    void OnMonsterAnimation(Protocol::AnimationData animationData);
    void OnMonsterHpChange(Protocol::HealthData healthData);
    void OnMonsterDespawn(uint64 monsterId);

    void OnDisconnect();
    void SetCameraFollower(ComponentRef<TargetFollower> cameraFollower);
    virtual int GetVersion() const override { return 10; }

    void PushJob(function<void(void)> func);
    GameState GetGameState() const { return _gameState; }
    void SetGameState(GameState gameState);
    void LoginState();
    void ConnectedState();

    void PlayImpulse(Vec3 position);

    Vec3 GetPlayerPosition() const;

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

        if (_version >= 4)
        {
            ar(CEREAL_NVP(_cameraSpawnPoint));
        }

        if (_version >= 5)
        {
            ar(CEREAL_NVP(_usernameInput));
        }

        if (_version >= 6)
        {
            ar(CEREAL_NVP(_hpChangeParticlePrefab));
        }

        if (_version >= 7)
        {
            ar(CEREAL_NVP(_impulseRenderer));
        }

        if (_version >= 8)
        {
            ar(CEREAL_NVP(_hudObj));
            ar(CEREAL_NVP(_shopObj));
        }

        if (_version >= 9)
        {
            ar(CEREAL_NVP(_hpBar));
            ar(CEREAL_NVP(_mpBar));
            ar(CEREAL_NVP(_spBar));
            ar(CEREAL_NVP(_coinText));
        }

        if (_version >= 10)
        {
            ar(CEREAL_NVP(_hpBarText));
            ar(CEREAL_NVP(_mpBarText));
            ar(CEREAL_NVP(_coinIcon));
        }
    }

private:
    void SpawnBloodParticle(Transform* target);

    inline static GameManager* _instance = nullptr;

    USE_LOCK
    vector<function<void(void)>> _jobQueue;

    ComponentRef<Player> _playerPrefab;
    unordered_map<uint64, ComponentRef<Player>> _players;
    ComponentRef<Player> _myPlayer;

    ComponentRef<Zombie> _zombiePrefab;
    ComponentRef<ParticleSystem> _hpChangeParticlePrefab;
    ComponentRef<TargetFollower> _cameraFollower;
    ComponentRef<Camera> _camera;
    unordered_map<uint64, ComponentRef<Zombie>> _monsters;

    ComponentRef<Transform> _titlePoint;
    ComponentRef<Transform> _playerSpawnPoint;
    ComponentRef<Transform> _cameraSpawnPoint;

    ComponentRef<UIImage> _titleImage;
    ComponentRef<Button> _loginButton;
    ComponentRef<InputText> _usernameInput;
    GameState _gameState = GameState::Title;

    ComponentRef<MeshRenderer> _impulseRenderer;

    GameObjectRef _hudObj;
    // regacy
    GameObjectRef _shopObj;

    ComponentRef<RectTransform> _hpBar;
    ComponentRef<Text> _hpBarText;
    ComponentRef<RectTransform> _mpBar;
    ComponentRef<Text> _mpBarText;
    ComponentRef<RectTransform> _spBar;
    ComponentRef<Text> _coinText;
    ComponentRef<UIImage> _coinIcon;
};

#define GM GameManager::GetInstance()
