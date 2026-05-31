#pragma once
#include <vector>
#include <string>
#include "ClientInfo.h"

enum class RoomState {
	WAITING,
	PLAYING,
	CLOSED
};

struct Room {
	int id;
	std::string roomName;
	std::vector<ClientInfo*> players;
	int maxPlayers;
	RoomState state = RoomState::WAITING;
};