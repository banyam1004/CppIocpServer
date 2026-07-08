#pragma once
#include <winsock2.h>

struct ClientInfo {
	OVERLAPPED overlapped;
	SOCKET socket;
	char buffer[1024];
	char name[50];
	bool isNameSet = false;
	WSABUF wsaBuf;
	char recvBuffer[4096];
	int recvSize = 0;
	
	int userId = 0;
	int level = 1;
	int hp = 100;
	int exp = 0;
	char charName[50] = "";
};