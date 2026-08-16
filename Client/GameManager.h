#pragma once
#include <MonoBehaviour.h>

class GameManager : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(GameManager)
public:
    static GameManager* GetInstance() { return _instance; }

    virtual void Awake();
    virtual void Update();

    void OnCreateMyPlayer(Protocol::Player myPlayer);
    void OnOtherPlayerEnter(Protocol::Player otherPlayer);
    void OnOtherPlayerExit(uint64 otherPlayerId);

    void PushJob(function<void(void)> func);

private:
    inline static GameManager* _instance = nullptr;

    USE_LOCK
    vector<function<void(void)>> _jobQueue;

    unordered_map<uint64, ComponentRef<class Player>> _players;
};

#define GM GameManager::GetInstance()
