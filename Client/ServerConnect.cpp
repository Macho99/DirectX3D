#include "pch.h"
#include "ServerConnect.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "Service.h"
#include "ThreadManager.h"

void ServerConnect::Awake()
{
	ASSERT(_instance == nullptr || _instance == this, "ServerConnect already exists");
	if (_instance != nullptr && _instance != this)
		return;

	_instance = this;

	ServerPacketHandler::Init();
	this_thread::sleep_for(1s);

	service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>,
		1
	);

    bool connected = service->Start();
    ASSERT_CRASH(connected, "Failed to connect to server");

    isRunning = true;
	for (int32 i = 0; i < 1; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (isRunning.load())
				{
					service->GetIocpCore()->Dispatch(100);
				}
			});
	}
}

void ServerConnect::OnDestroy()
{
	if (_instance != this)
		return;

	isRunning.store(false);
	GThreadManager->Join();
	service = nullptr;
	_instance = nullptr;
}

bool ServerConnect::OnGUI()
{
	bool changed = false;

    const bool enterPressed = ImGui::InputText(
        "Chat", chatStr, IM_ARRAYSIZE(chatStr), ImGuiInputTextFlags_EnterReturnsTrue);
    changed |= enterPressed;

    ImGui::SameLine();
    const bool buttonPressed = ImGui::Button("Send Chat");

    if ((enterPressed || buttonPressed) && chatStr[0] != '\0')
    {
		Protocol::C_CHAT chatPkt;
		chatPkt.set_msg(chatStr);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(chatPkt);

		service->Broadcast(sendBuffer);
		chatStr[0] = '\0';
    }

	return changed;
}
