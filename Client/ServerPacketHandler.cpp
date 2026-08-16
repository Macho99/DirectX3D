#include "pch.h"
#include "ServerPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return true;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	//cout << "Handle_S_LOGIN" << endl;
	if (pkt.success() == false)
		return true;

	auto Job = [pkt]()
		{
			GM->OnCreateMyPlayer(pkt.myplayer());

			for (int i = 0; i < pkt.otherplayers_size(); i++)
			{
				GM->OnOtherPlayerEnter(pkt.otherplayers(i));
			}
		};

	GM->PushJob(Job);

	return true;
}

bool Handle_S_PLAYER_ENTER(PacketSessionRef& session, Protocol::S_PLAYER_ENTER& pkt)
{
	return true;
}

bool Handle_S_PLAYER_EXIT(PacketSessionRef& session, Protocol::S_PLAYER_EXIT& pkt)
{
	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	//cout << "Handle_S_CHAT" << endl;
	std::cout << pkt.msg() << endl;
	return true;
}