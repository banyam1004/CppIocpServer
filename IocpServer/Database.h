#pragma once
#include <mysql.h>
#include <iostream>
#include <string>

inline MYSQL* dbConn = nullptr;

bool connectDB(const char* host, const char* user,
	const char* password, const char* dbName) {
	dbConn = mysql_init(NULL);
	if (dbConn == NULL) {
		std::cerr << "mysql_init() failed\n";
		return false;
	}

	if (mysql_real_connect(dbConn, host, user, password,
		dbName, 3306, NULL, 0) == NULL) {
		std::cerr << "mysql_real_connect() failed: "
			<< mysql_error(dbConn) << "\n";
		mysql_close(dbConn);
		return false;
	}

	std::cout << "MySQL Connected!\n";
	return true;
}

void disconnectDB() {
	if (dbConn != nullptr) {
		mysql_close(dbConn);
		dbConn = nullptr;
		std::cout << "MySQL Disconnected!\n";
	}
}

bool registerUser(const std::string& username, const std::string& password) {
	std::string query = "INSERT INTO users (username, password) VALUES ('"
		+ username + "', '" + password + "')";

	if (mysql_query(dbConn, query.c_str()) != 0) {
		std::cerr << "registeruser() failed: " << mysql_error(dbConn) << "\n";
		return false;
	}
	return true;
}

bool loginUser(const std::string& username, const std::string& password) {
	std::string query = "SELECT id FROM users WHERE username='"
		+ username + "' AND password='" + password + "'";

	if (mysql_query(dbConn, query.c_str()) != 0) {
		std::cerr << "loginUser() failed: " << mysql_error(dbConn) << "\n";
		return false;
	}

	MYSQL_RES* result = mysql_store_result(dbConn);
	if (result == NULL) return false;
	bool success = (mysql_num_rows(result) > 0);
	mysql_free_result(result);
	return success;
}