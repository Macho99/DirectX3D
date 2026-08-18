#include "pch.h"
#include "ServerConnect.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "Service.h"
#include "ThreadManager.h"
#include "OnGUIUtils.h"

void ServerConnect::Awake()
{
	ASSERT(_instance == nullptr || _instance == this, "ServerConnect already exists");
	if (_instance != nullptr && _instance != this)
		return;

	_instance = this;
}

void ServerConnect::OnDestroy()
{
	if (_instance != this)
		return;

	_isRunning.store(false);
	GThreadManager->Join();
	_service = nullptr;
	_instance = nullptr;
}

bool ServerConnect::OnGUI()
{
	bool changed = false;

	changed = OnGUIUtils::DrawString("DebugChat", &_debugChat);
    ImGui::SameLine();
    if (ImGui::Button("Send Chat") && _debugChat.empty() == false)
    {
		Protocol::C_CHAT chatPkt;
		chatPkt.set_msg(_debugChat.c_str());
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(chatPkt);

		_service->Broadcast(sendBuffer);
		_debugChat = "";
    }

	if (_isRunning)
	{
		if (ImGui::Button("Disconnect"))
		{
			Disconnect();
		}

		if (ImGui::Button("LogIn"))
		{
			Protocol::C_LOGIN pkt;
			pkt.set_name(_debugChat);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
			_service->Broadcast(sendBuffer);
		}
	}
	else
	{
		if (ImGui::Button("Connect"))
		{
			TryConnect();
		}
	}

	return changed;
}

bool ServerConnect::TryConnect()
{
	if (_isRunning)
		return true;

	ServerPacketHandler::Init();
	this_thread::sleep_for(1s);

	_service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>,
		1
	);

	bool connected = _service->Start();
	if (connected == false)
	{
		return connected;
	}

	_isRunning = true;
	for (int32 i = 0; i < 1; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (_isRunning.load())
				{
					_service->GetIocpCore()->Dispatch(100);
				}
			});
	}

	return connected;
}

void ServerConnect::Disconnect()
{
	if (_isRunning == false)
		return;

	auto& sessions = _service->GetSessions();
	for (auto session : sessions)
	{
		session->Disconnect(L"LogOut");
	}

	_isRunning.store(false);
	GThreadManager->Join();
	_service = nullptr;
}

void ServerConnect::SendPacket(SendBufferRef sendBuffer)
{
	if (_service == nullptr)
	{
		DBG->LogError("Service is null. Cannot send packet.");
		return;
	}

	if (_isRunning == false)
	{
		DBG->LogError("Service is not running. Cannot send packet.");
		return;
	}

	_service->Broadcast(sendBuffer);
}
