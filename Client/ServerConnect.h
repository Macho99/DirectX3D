#pragma once
#include "MonoBehaviour.h"
class ServerConnect : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(ServerConnect)

public:
    static ServerConnect* GetInstance() { return _instance; }

    virtual void Awake() override;
    virtual void OnDestroy() override;
    virtual bool OnGUI() override;

    bool TryConnect();
    void Disconnect();
    void SendPacket(SendBufferRef sendBuffer);

private:
    inline static ServerConnect* _instance = nullptr;

    string _debugChat;
    ClientServiceRef _service;
    Atomic<bool> _isRunning = false;
};

#define SERVER_CONNECT ServerConnect::GetInstance()

