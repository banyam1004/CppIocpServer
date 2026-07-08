#pragma once
#include <iostream>
#include <winsock2.h>
#include <algorithm>
#include "ClientInfo.h"
#include "Broadcast.h"
#include "RoomManager.h"
#include "Logger.h"
#include "Packet.h"
#include "Database.h"

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

		std::cout << "clientInfo address: " << clientInfo << "\n";
		std::cout << "clientInfo->userId: " << clientInfo->userId << "\n";

		if (!result || bytesTransferred == 0) {
			std::cout << "Client disconnected!\n";
			std::cout << "result: " << result << " bytesTransferred: " << bytesTransferred << "\n";

			if (clientInfo->userId > 0) {
				std::cout << "Saving chracter...\n";
				saveCharacter(
					clientInfo->userId,
					clientInfo->level,
					clientInfo->hp,
					clientInfo->exp
				);
				writeLog(std::string(clientInfo->name) + " character saved");
			}

			{
				std::lock_guard<std::mutex> lock(clientsMutex);
				clients.erase(
					std::remove(clients.begin(), clients.end(), clientInfo),
					clients.end()
				);
			}
			{
				std::lock_guard<std::mutex> lock(roomsMutex);
				for (Room* room : rooms) {
					auto it = std::find(room->players.begin(), room->players.end(), clientInfo);
					if (it != room->players.end()) {
						room->players.erase(it);
						if (room->players.empty()) {
							rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());
							delete room;
						}
						break;
					}
				}
			}
			writeLog(std::string(clientInfo->name) + " disconneted");

			closesocket(clientInfo->socket);
			delete clientInfo;
			continue;
		}

		memcpy(clientInfo->recvBuffer + clientInfo->recvSize, clientInfo->buffer, bytesTransferred);
		clientInfo->recvSize += bytesTransferred;

		while (true) {
			if (clientInfo->recvSize < sizeof(PacketHeader)) break;

			PacketHeader header = parseHeader(clientInfo->recvBuffer);
			int totalSize = header.size;

			if (clientInfo->recvSize < totalSize) break;

			std::string data = parseData(clientInfo->recvBuffer, totalSize);

			if (!clientInfo->isNameSet) {
				std::string received = data;
				int colonPos = received.find(':');

				if (colonPos == std::string::npos) {
					sendPacket(clientInfo->socket, PacketType::NOTIFY, "Invalid format. Use id:password");
				}
				else {
					std::string username = received.substr(0, colonPos);
					std::string password = received.substr(colonPos + 1);

					if (loginUser(username, password)) {
						strncpy_s(clientInfo->name, username.c_str(), sizeof(clientInfo->name) - 1);
						clientInfo->isNameSet = true;

						clientInfo->userId = getUserId(username);
						std::cout << "userId set: " << clientInfo->userId << "\n";

						sendPacket(clientInfo->socket, PacketType::NOTIFY, "Login success! Welcome " + username);
						std::string welcomMsg = username + " joined!";
						broadcast(PacketType::NOTIFY, welcomMsg, clientInfo->socket);
						writeLog(username + " logged in");
					}
					else {
						if (registerUser(username, password)) {
							strncpy_s(clientInfo->name, username.c_str(), sizeof(clientInfo->name) - 1);
							clientInfo->isNameSet = true;

							clientInfo->userId = getUserId(username);

							loadOrCreateCharacter(
								clientInfo->userId,
								clientInfo->charName,
								clientInfo->level,
								clientInfo->hp,
								clientInfo->exp
							);

							std::string charInfo = "Level: " + std::to_string(clientInfo->level) +
								" | HP: " + std::to_string(clientInfo->hp) +
								" | EXP: " + std::to_string(clientInfo->exp);

							sendPacket(clientInfo->socket, PacketType::NOTIFY, "Login success! Welcome " + username);
							sendPacket(clientInfo->socket, PacketType::NOTIFY, charInfo);
							std::string welcomeMsg = username + " joined!";
							broadcast(PacketType::NOTIFY, welcomeMsg, clientInfo->socket);
							writeLog(username + " registered");
						}
						else {
							sendPacket(clientInfo->socket, PacketType::NOTIFY, "Login failed.");
						}
					}
				}
				//broadcast 추가하기
			}
			else {
				switch (header.type) {
				case PacketType::CHAT: {
					std::string fullMsg = std::string(clientInfo->name) + ": " + data;
					writeLog(fullMsg);
					broadcast(PacketType::CHAT, fullMsg, clientInfo->socket);

					clientInfo->exp += 10;
					std::string expMsg = "EXP: " + std::to_string(clientInfo->exp);
					sendPacket(clientInfo->socket, PacketType::NOTIFY, expMsg);

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
			int remaining = clientInfo->recvSize - totalSize;
			memmove(clientInfo->recvBuffer, clientInfo->recvBuffer + totalSize, remaining);
			clientInfo->recvSize = remaining;
		}
		DWORD flags = 0;
		clientInfo->wsaBuf.buf = clientInfo->buffer;
		clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);
		memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
		WSARecv(clientInfo->socket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
	}
}