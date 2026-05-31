#pragma once
#include <iostream>
#include <winsock2.h>
#include "ClientInfo.h"
#include "Broadcast.h"

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

			char welcomeMsg[100];
			snprintf(welcomeMsg, sizeof(welcomeMsg), "%s joined!", clientInfo->name);
			broadcast(welcomeMsg, strlen(welcomeMsg), clientInfo->socket);
		}
		else {
			char fullMsg[1100];
			snprintf(fullMsg, sizeof(fullMsg), "%s: %s", clientInfo->name, clientInfo->buffer);
			std::cout << fullMsg << "\n";
			broadcast(fullMsg, strlen(fullMsg), clientInfo->socket);
		}

		DWORD flags = 0;
		clientInfo->wsaBuf.buf = clientInfo->buffer;
		clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);
		memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
		WSARecv(clientInfo->socket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
	}
}