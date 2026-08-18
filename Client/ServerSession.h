#pragma once
#include "ThreadManager.h"
#include <chrono>
#include "BufferReader.h"
#include "ServerPacketHandler.h"

#include "Service.h"
#include "Session.h"
#include "ServerConnect.h"

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

public:
	virtual void OnConnected() override
	{
        DBG->Log("ServerSession::OnConnected");
		GM->ConnectedState();
	}

	virtual void OnDisconnected() override
	{
        DBG->Log("ServerSession::OnDisconnected");
		GM->OnDisconnect();
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// TODO : packetId 대역 체크
		ServerPacketHandler::HandlePacket(session, buffer, len);
	}
};
