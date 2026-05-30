#pragma once
#include <iostream>
#include <winsock2.h>
#include "ClientInfo.h"

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
		std::cout << "Client: " << clientInfo->buffer << "\n";

		DWORD flags = 0;
		clientInfo->wsaBuf.buf = clientInfo->buffer;
		clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);
		memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
		WSARecv(clientInfo->socket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
	}
}