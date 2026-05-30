#pragma once
#include <winsock2.h>

struct ClientInfo {
	OVERLAPPED overlapped;
	SOCKET socket;
	char buffer[1024];
	WSABUF wsaBuf;
};