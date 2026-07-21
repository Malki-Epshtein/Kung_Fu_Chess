#include "UserRepository.h"
#include "sqlite3.h"
#include <functional>
#include <stdexcept>

namespace {
    constexpr int kStartingElo = 1200;

    // Not cryptographically secure - this is a local/student project, not a
    // production auth system. Good enough that raw passwords never sit in
    // the .db file on disk.
    std::string hashPassword(const std::string& password) {
        return std::to_string(std::hash<std::string>{}(password));
    }
}

UserRepository::UserRepository(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        std::string message = "Cannot open user database: " + std::string(sqlite3_errmsg(db_));
        sqlite3_close(db_);
        throw std::runtime_error(message);
    }

    const char* createTableSql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  username TEXT PRIMARY KEY,"
        "  password TEXT NOT NULL,"
        "  elo INTEGER NOT NULL DEFAULT 1200"
        ");";
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, createTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string message = "Cannot create users table: " + std::string(errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db_);
        throw std::runtime_error(message);
    }
}

UserRepository::~UserRepository() {
    sqlite3_close(db_);
}

LoginResult UserRepository::login(const std::string& username, const std::string& password) {
    std::string hashed = hashPassword(password);

    sqlite3_stmt* select = nullptr;
    sqlite3_prepare_v2(db_, "SELECT password, elo FROM users WHERE username = ?;", -1, &select, nullptr);
    sqlite3_bind_text(select, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(select) == SQLITE_ROW) {
        std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(select, 0));
        int elo = sqlite3_column_int(select, 1);
        sqlite3_finalize(select);

        if (storedHash != hashed)
            return { false, "wrong password", 0 };
        return { true, "login successful", elo };
    }
    sqlite3_finalize(select);

    // No existing account under this username - create one.
    sqlite3_stmt* insert = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO users(username, password, elo) VALUES (?, ?, ?);", -1, &insert, nullptr);
    sqlite3_bind_text(insert, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insert, 2, hashed.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(insert, 3, kStartingElo);
    bool inserted = sqlite3_step(insert) == SQLITE_DONE;
    sqlite3_finalize(insert);

    if (!inserted)
        return { false, "could not create account", 0 };
    return { true, "account created", kStartingElo };
}

void UserRepository::updateElo(const std::string& username, int newElo) {
    sqlite3_stmt* update = nullptr;
    sqlite3_prepare_v2(db_, "UPDATE users SET elo = ? WHERE username = ?;", -1, &update, nullptr);
    sqlite3_bind_int(update, 1, newElo);
    sqlite3_bind_text(update, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(update);
    sqlite3_finalize(update);
}
