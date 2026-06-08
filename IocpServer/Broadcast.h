#pragma once
#include <winsock2.h>
#include <vector>
#include <mutex>
#include "ClientInfo.h"
#include "Packet.h"

inline std::vector<ClientInfo*> clients;
inline std::mutex clientsMutex;

void broadcast(PacketType type, const std::string& message, SOCKET sender) {
	std::lock_guard<std::mutex> lock(clientsMutex);
	for (ClientInfo* client : clients) {
		if (client->socket != sender) {
			sendPacket(client->socket, type, message);
		}
	}

}