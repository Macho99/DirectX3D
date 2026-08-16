#pragma once
#include "MonoBehaviour.h"
class ServerConnect : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(ServerConnect)
public:
    virtual void Awake() override;
    virtual void OnDestroy() override;

    virtual bool OnGUI() override;

private:
    char chatStr[256] = {};
    ClientServiceRef service;
    Atomic<bool> isRunning = false;
};

