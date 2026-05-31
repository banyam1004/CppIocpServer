#pragma once
#include <winsock2.h>
#include <vector>
#include <mutex>
#include "ClientInfo.h"

inline std::vector<ClientInfo*> clients;
inline std::mutex clientsMutex;

void broadcast(const char* buffer, int len, SOCKET sender) {
	std::lock_guard<std::mutex> lock(clientsMutex);
	for (ClientInfo* client : clients) {
		if (client->socket != sender) {
			send(client->socket, buffer, len, 0);
		}
	}

}