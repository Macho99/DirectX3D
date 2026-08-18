#include "pch.h"
#include "ServerPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    DBG->LogError("Invalid Packet ID : 0x%04X, Size : %d", header->id, header->size);
	return true;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
    DBG->Log("Handle_S_LOGIN");
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
	DBG->Log("Handle_S_PLAYER_ENTER");

    auto Job = [pkt]()
        {
			for (int i = 0; i < pkt.players_size(); i++)
			{
				GM->OnOtherPlayerEnter(pkt.players(i));
			}
        };

    GM->PushJob(Job);

	return true;
}

bool Handle_S_PLAYER_EXIT(PacketSessionRef& session, Protocol::S_PLAYER_EXIT& pkt)
{
	DBG->Log("Handle_S_PLAYER_EXIT");

    auto Job = [pkt]()
        {
			for (int i = 0; i < pkt.playerids_size(); i++)
				GM->OnOtherPlayerExit(pkt.playerids(i));
        };

    GM->PushJob(Job);

	return true;
}

bool Handle_S_PLAYER_MOVE(PacketSessionRef& session, Protocol::S_PLAYER_MOVE& pkt)
{
    auto Job = [pkt]()
        {
            for (int i = 0; i < pkt.transforms_size(); i++)
            {
                GM->OnMovePlayer(pkt.transforms(i));
            }
        };

	GM->PushJob(Job);

	return true;
}

bool Handle_S_MONSTER_SPAWN(PacketSessionRef& session, Protocol::S_MONSTER_SPAWN& pkt)
{
	return true;
}

bool Handle_S_MONSTER_MOVE(PacketSessionRef& session, Protocol::S_MONSTER_MOVE& pkt)
{
    auto Job = [pkt]()
        {
            for (int i = 0; i < pkt.transforms_size(); i++)
            {
                GM->OnMoveMonster(pkt.transforms(i));
            }
        };
	GM->PushJob(Job);

	return true;
}

bool Handle_S_MONSTER_DESPAWN(PacketSessionRef& session, Protocol::S_MONSTER_DESPAWN& pkt)
{
	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	DBG->Log("Handle_S_CHAT");
	std::cout << pkt.msg() << endl;
	return true;
}