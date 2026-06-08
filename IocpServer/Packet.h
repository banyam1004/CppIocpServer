#pragma once
#include <cstdint>
#include <string>
#include <winsock2.h>
#include <vector>

enum class PacketType : uint16_t {
	CHAT = 1,
	CREATE = 2,
	JOIN = 3,
	LEAVE = 4,
	LIST = 5,
	NOTIFY = 6
};

#pragma pack(push, 1)
struct PacketHeader {
	PacketType type;
	uint16_t size;
};
#pragma pack(pop)

void sendPacket(SOCKET socket, PacketType type, const std::string& data) {
	PacketHeader header;
	header.type = type;
	header.size = sizeof(PacketHeader) + data.size();

	std::vector<char> packet(header.size);
	memcpy(packet.data(), &header, sizeof(PacketHeader));
	memcpy(packet.data() + sizeof(PacketHeader), data.c_str(), data.size());

	send(socket, packet.data(), header.size, 0);
}

PacketHeader parseHeader(const char* buffer) {
	PacketHeader header;
	memcpy(&header, buffer, sizeof(PacketHeader));
	return header;
}

std::string parseData(const char* buffer, int totalSize) {
	int dataSize = totalSize - sizeof(PacketHeader);
	return std::string(buffer + sizeof(PacketHeader), dataSize);
}