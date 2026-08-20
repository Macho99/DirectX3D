#pragma once
#include "Protocol.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C_LOGIN = 1000,
	PKT_S_LOGIN = 1001,
	PKT_S_PLAYER_ENTER = 1002,
	PKT_S_PLAYER_EXIT = 1003,
	PKT_C_PLAYER_INPUT = 1004,
	PKT_S_PLAYER_ANIMATION = 1005,
	PKT_S_PLAYER_MOVE = 1006,
	PKT_S_MONSTER_SPAWN = 1007,
	PKT_S_MONSTER_MOVE = 1008,
	PKT_S_MONSTER_ANIMATION = 1009,
	PKT_S_MONSTER_DESPAWN = 1010,
	PKT_C_CHAT = 1011,
	PKT_S_CHAT = 1012,
	PKT_C_SPAWN_MONSTER = 1013,
};

// Custom Handlers
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt);
bool Handle_S_PLAYER_ENTER(PacketSessionRef& session, Protocol::S_PLAYER_ENTER& pkt);
bool Handle_S_PLAYER_EXIT(PacketSessionRef& session, Protocol::S_PLAYER_EXIT& pkt);
bool Handle_S_PLAYER_ANIMATION(PacketSessionRef& session, Protocol::S_PLAYER_ANIMATION& pkt);
bool Handle_S_PLAYER_MOVE(PacketSessionRef& session, Protocol::S_PLAYER_MOVE& pkt);
bool Handle_S_MONSTER_SPAWN(PacketSessionRef& session, Protocol::S_MONSTER_SPAWN& pkt);
bool Handle_S_MONSTER_MOVE(PacketSessionRef& session, Protocol::S_MONSTER_MOVE& pkt);
bool Handle_S_MONSTER_ANIMATION(PacketSessionRef& session, Protocol::S_MONSTER_ANIMATION& pkt);
bool Handle_S_MONSTER_DESPAWN(PacketSessionRef& session, Protocol::S_MONSTER_DESPAWN& pkt);
bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_S_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN>(Handle_S_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYER_ENTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PLAYER_ENTER>(Handle_S_PLAYER_ENTER, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYER_EXIT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PLAYER_EXIT>(Handle_S_PLAYER_EXIT, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYER_ANIMATION] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PLAYER_ANIMATION>(Handle_S_PLAYER_ANIMATION, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYER_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PLAYER_MOVE>(Handle_S_PLAYER_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_MONSTER_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MONSTER_SPAWN>(Handle_S_MONSTER_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_MONSTER_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MONSTER_MOVE>(Handle_S_MONSTER_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_MONSTER_ANIMATION] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MONSTER_ANIMATION>(Handle_S_MONSTER_ANIMATION, session, buffer, len); };
		GPacketHandler[PKT_S_MONSTER_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MONSTER_DESPAWN>(Handle_S_MONSTER_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT>(Handle_S_CHAT, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::C_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_C_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PLAYER_INPUT& pkt) { return MakeSendBuffer(pkt, PKT_C_PLAYER_INPUT); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_C_CHAT); }
	static SendBufferRef MakeSendBuffer(Protocol::C_SPAWN_MONSTER& pkt) { return MakeSendBuffer(pkt, PKT_C_SPAWN_MONSTER); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);

		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;

		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));

		sendBuffer->Close(packetSize);
		return sendBuffer;
	}
};