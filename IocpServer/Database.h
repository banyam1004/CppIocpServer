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

int getUserId(const std::string& username) {
	std::string query = "SELECT id FROM users WHERE username='" + username + "'";

	if (mysql_query(dbConn, query.c_str()) != 0) return -1;

	MYSQL_RES* result = mysql_store_result(dbConn);
	if (result == NULL) return -1;

	MYSQL_ROW row = mysql_fetch_row(result);
	int id = (row != NULL) ? atoi(row[0]) : -1;
	mysql_free_result(result);
	return id;
}

bool loadOrCreateCharacter(int userId, char* name, int& level, int& hp, int& exp) {
	std::string query = "SELECT name, level, hp, exp FROM characters WHERE user_id=" + std::to_string(userId);
	if (mysql_query(dbConn, query.c_str()) != 0) return false;

	MYSQL_RES* result = mysql_store_result(dbConn);
	if (result == NULL) return false;

	MYSQL_ROW row = mysql_fetch_row(result);

	if (row != NULL) {
		strcpy_s(name, 50, row[0]);
		level = atoi(row[1]);
		hp = atoi(row[2]);
		exp = atoi(row[3]);
		mysql_free_result(result);
	}
	else {
		mysql_free_result(result);
		std::string insert = "INSERT INTO characters (user_id, name, level, hp, exp) VALUES ("
			+ std::to_string(userId) + ", '" + name + "', 1, 100, 0)";
		if (mysql_query(dbConn, insert.c_str()) != 0) return false;
		level = 1;
		hp = 100;
		exp = 0;
	}
	return true;
}

bool saveCharacter(int userId, int level, int hp, int exp) {
	std::string query = "UPDATE characters SET level=" + std::to_string(level) +
		", hp=" + std::to_string(hp) +
		", exp=" + std::to_string(exp) +
		" WHERE user_id=" + std::to_string(userId);

	if (mysql_query(dbConn, query.c_str()) != 0) {
		std::cerr << "saveChracter() failed: " << mysql_error(dbConn) << "\n";
		return false;
	}
	return true;
}