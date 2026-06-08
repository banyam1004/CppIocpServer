#pragma once
#include <iostream>
#include <winsock2.h>
#include <algorithm>
#include "ClientInfo.h"
#include "Broadcast.h"
#include "RoomManager.h"
#include "Logger.h"

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
			strncpy_s(clientInfo->name, clientInfo->buffer, sizeof(clientInfo->name) - 1);
			clientInfo->isNameSet = true;
			std::cout << clientInfo->name << " conneted!\n";
			writeLog(std::string(clientInfo->name) + " connected");

			char welcomeMsg[100];
			snprintf(welcomeMsg, sizeof(welcomeMsg), "%s joined!", clientInfo->name);
			broadcast(welcomeMsg, strlen(welcomeMsg), clientInfo->socket);
		}
		else {
			if (strncmp(clientInfo->buffer, "/create ", 8) == 0) {
				char roomName[50];
				int maxPlayers;
				sscanf_s(clientInfo->buffer + 8, "%s %d", roomName, (unsigned)sizeof(roomName), &maxPlayers);

				Room* room = createRoom(roomName, maxPlayers, clientInfo);

				char msg[100];
				snprintf(msg, sizeof(msg), "Room [%d] %s created! (max: %d)",
					room->id,
					roomName,
					maxPlayers
				);
				send(clientInfo->socket, msg, strlen(msg), 0);
				writeLog(std::string(clientInfo->name) + " created room");
			}
			else if (strncmp(clientInfo->buffer, "/join ", 6) == 0) {
				int roomId;
				sscanf_s(clientInfo->buffer + 6, "%d", &roomId);

				if (joinRoom(roomId, clientInfo)) {
					char msg[100];
					snprintf(msg, sizeof(msg), "Joined room [%d]!", roomId);
					send(clientInfo->socket, msg, strlen(msg), 0);

					std::lock_guard<std::mutex> lock(roomsMutex);
					for (Room* room : rooms) {
						if (room->id == roomId) {
							char notify[100];
							snprintf(notify, sizeof(notify), "%s joined the room!", clientInfo->name);
							for (ClientInfo* member : room->players) {
								if (member->socket != clientInfo->socket) {
									send(member->socket, notify, strlen(notify), 0);
								}
							}
							break;
						}
					}
				}
				else {
					char msg[] = "Failed to join room. (Full or not found)";
					send(clientInfo->socket, msg, strlen(msg), 0);
				}
			}
			else if (strncmp(clientInfo->buffer, "/list", 5) == 0) {
				if (rooms.empty()) {
					char msg[] = "No rooms available.";
					send(clientInfo->socket, msg, strlen(msg), 0);
				}
				else {
					char msg[1024] = "=== Room List ===\n";
					for(Room* room : rooms) {
						char roomInfo[256];
						snprintf(roomInfo, sizeof(roomInfo), "[%d] %s (%d/%d)\n",
							room->id,
							room->roomName.c_str(),
							(int)room->players.size(),
							room->maxPlayers
						);
						strcat_s(msg, sizeof(msg), roomInfo);
					}
					send(clientInfo->socket, msg, strlen(msg), 0);
				}
			}
			else if (strncmp(clientInfo->buffer, "/leave", 6) == 0) {
				int roomId;
				char leaveMsg[256];
				bool found = false;
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
							char msg[256] = "you left the room";
							snprintf(leaveMsg, sizeof(leaveMsg), "%s is leave", clientInfo->name);
							send(clientInfo->socket, msg, strlen(msg), 0);
							found = true;
							break;
						}
					}
					if (found) break;
				}
				for (Room* room : rooms) {
					if(room->id == roomId){
						for (ClientInfo* member : room->players) {
							send(member->socket, leaveMsg, sizeof(leaveMsg), 0);
						}
					}
				}
			}
			else {
				char fullMsg[1100];
				snprintf(fullMsg, sizeof(fullMsg), "%s: %s", clientInfo->name, clientInfo->buffer);
				std::cout << fullMsg << "\n";
				broadcast(fullMsg, strlen(fullMsg), clientInfo->socket);
				writeLog(std::string(clientInfo->name) + ": " + clientInfo->buffer);
			}
		}

		DWORD flags = 0;
		clientInfo->wsaBuf.buf = clientInfo->buffer;
		clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);
		memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
		WSARecv(clientInfo->socket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
	}
}