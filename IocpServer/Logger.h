#pragma once
#include <fstream>
#include <ctime>
#include <string>
#include <mutex>

inline std::mutex logMutex;

std::string getCurrentTime() {
	time_t timer = time(NULL);
	struct tm timeInfo;
	localtime_s(&timeInfo, &timer);

	char timeStamp[100];
	snprintf(timeStamp, sizeof(timeStamp), "%d-%02d-%02d %02d:%02d:%02d",
		timeInfo.tm_year + 1900,
		timeInfo.tm_mon + 1,
		timeInfo.tm_mday,
		timeInfo.tm_hour,
		timeInfo.tm_min,
		timeInfo.tm_sec
	);
	return timeStamp;
}

void writeLog(const std::string& message) {
	std::lock_guard<std::mutex> lock(logMutex);
	std::ofstream logFile("server.log", std::ios::app);
	logFile << getCurrentTime() << " " << message << "\n";
}