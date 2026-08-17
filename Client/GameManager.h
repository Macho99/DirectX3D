#pragma once
#include <MonoBehaviour.h>

class Player;
class Zombie;
class TargetFollower;

class GameManager : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(GameManager)
public:
    static GameManager* GetInstance() { return _instance; }

    virtual void Awake();
    virtual void Update();
    virtual void OnDestroy();

    void OnCreateMyPlayer(Protocol::Player myPlayer);
    void OnOtherPlayerEnter(Protocol::Player otherPlayer);
    void OnOtherPlayerExit(uint64 otherPlayerId);
    void OnDisconnect();
    void SetCameraFollower(ComponentRef<TargetFollower> cameraFollower);

    void PushJob(function<void(void)> func);

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_playerPrefab));
        ar(CEREAL_NVP(_zombiePrefab));
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
};

#define GM GameManager::GetInstance()
