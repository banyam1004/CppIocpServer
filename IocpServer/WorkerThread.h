#pragma once
#include <iostream>
#include <winsock2.h>
#include <algorithm>
#include "ClientInfo.h"
#include "Broadcast.h"
#include "RoomManager.h"
#include "Logger.h"
#include "Packet.h"

void workerThread(HANDLE iocpHandle) {
	while (true) {
		DWORD bytesTransferred;
		ClientInfo* clientInfo = nullptr;
		ULONG_PTR completionKey;

		BOOL result = GetQueuedCompletionStatus(
			iocpHandle,
			&bytesTransferred,
			&completionKey,
			(OVERLAPPED**)&clientInfo,
			INFINITE
		);

		if (!result || bytesTransferred == 0) {
			std::cout << "Client disconnected!\n";
			closesocket(clientInfo->socket);
			delete clientInfo;
			continue;
		}

		clientInfo->buffer[bytesTransferred] = '\0';

		if (!clientInfo->isNameSet) {
			PacketHeader header = parseHeader(clientInfo->buffer);
			std::string data = parseData(clientInfo->buffer, bytesTransferred);

			strncpy_s(clientInfo->name, data.c_str(), sizeof(clientInfo->name) - 1);
			clientInfo->isNameSet = true;

			std::string welcomeMsg = std::string(clientInfo->name) + " joined!";
			broadcast(PacketType::NOTIFY, welcomeMsg, clientInfo->socket);

			//broadcast 추가하기
		}
		else {
			PacketHeader header = parseHeader(clientInfo->buffer);
			std::string data = parseData(clientInfo->buffer, bytesTransferred);


			switch (header.type) {
			case PacketType::CHAT: {
				std::string fullMsg = std::string(clientInfo->name) + ": " + data;
				writeLog(fullMsg);
				broadcast(PacketType::CHAT, fullMsg, clientInfo->socket);
				break;
			}
			case PacketType::CREATE: {
				char roomName[50];
				int maxPlayers;
				sscanf_s(data.c_str(), "%s %d", roomName, (unsigned)sizeof(roomName), &maxPlayers);
				Room* room = createRoom(roomName, maxPlayers, clientInfo);
				std::string msg = "Room [" + std::to_string(room->id) + "] " + roomName + " created!";
				sendPacket(clientInfo->socket, PacketType::NOTIFY, msg);
				writeLog(std::string(clientInfo->name) + " created room");
				break;
			}
			case PacketType::JOIN: {
				int roomId = std::stoi(data);
				if (joinRoom(roomId, clientInfo)) {
					sendPacket(clientInfo->socket, PacketType::NOTIFY, "Joined room!");
				}
				else {
					sendPacket(clientInfo->socket, PacketType::NOTIFY, "Failed to join room.");
				}
				break;
			}
			case PacketType::LIST: {
				std::string msg = "=== Room List ===\n";
				if (rooms.empty()) {
					msg = "No rooms available.";
				}
				else {
					for (Room* room : rooms) {
						msg += "[" + std::to_string(room->id) + "]" + room->roomName +
							" (" + std::to_string(room->players.size()) + "/" +
							std::to_string(room->maxPlayers) + ")\n";
					}
				}
				sendPacket(clientInfo->socket, PacketType::NOTIFY, msg);
				break;
			}
			case PacketType::LEAVE: {
				std::string leaveMsg;
				bool found = false;
				int roomId = -1;
				{
					std::lock_guard<std::mutex> lock(roomsMutex);
					for (Room* room : rooms) {
						for (ClientInfo* member : room->players) {
							if (member->socket == clientInfo->socket) {
								roomId = room->id;
								room->players.erase(
									std::remove(room->players.begin(), room->players.end(), clientInfo),
									room->players.end()
								);
								if (room->players.empty()) {
									rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());
									delete room;
								}
								leaveMsg = std::string(clientInfo->name) + " left the room!";
								sendPacket(clientInfo->socket, PacketType::NOTIFY, "You left the room!");
								found = true;
								break;
							}
						}
						if (found) break;
					}
					if (found && roomId != -1) {
						for (Room* room : rooms) {
							if (room->id == roomId) {
								for (ClientInfo* member : room->players) {
									sendPacket(member->socket, PacketType::NOTIFY, leaveMsg);
								}
							}
						}
					}
				}
			}
			}
		}
		DWORD flags = 0;
		clientInfo->wsaBuf.buf = clientInfo->buffer;
		clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);
		memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
		WSARecv(clientInfo->socket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
	}
}