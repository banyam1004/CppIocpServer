#pragma once
#include <vector>
#include <mutex>
#include "Room.h"

inline std::vector<Room*> rooms;
inline std::mutex roomsMutex;
inline int nextRoomId = 1;

Room* createRoom(std::string name, int maxPlayers, ClientInfo* player) {
	std::lock_guard<std::mutex> lock(roomsMutex);

	Room* room = new Room();
	room->id = nextRoomId++;
	room->roomName = name;
	room->maxPlayers = maxPlayers;
	room->players.push_back(player);

	rooms.push_back(room);
	return room;
}

bool joinRoom(int id, ClientInfo* player) {
	std::lock_guard<std::mutex> lock(roomsMutex);

	Room* findRoom = nullptr;
	for (Room* room : rooms) {
		if (room->id == id) {
			findRoom = room;
			break;
		}
	}
	if (findRoom == nullptr) {
		return false;
	}
	if (findRoom->maxPlayers == findRoom->players.size()) {
		return false;
	}
	else {
		findRoom->players.push_back(player);
		return true;
	}
}

void deleteRoom(int id) {
	std::lock_guard<std::mutex> lock(roomsMutex);

	for (int i = 0; i < rooms.size(); i++) {
		if (rooms[i]->id == id) {
			delete rooms[i];
			rooms.erase(rooms.begin() + i);
			return;
		}
	}
}