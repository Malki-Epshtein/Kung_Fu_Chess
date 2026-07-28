#pragma once
#include "IUserRepository.h"
#include <string>

struct sqlite3;

// The only thing in this codebase that touches SQL directly - wraps the
// SQLite-backed users table (username, password, elo). CommandDispatcher/
// WsServer never see a query directly, matching how BoardParser/
// BoardPrinter are the only things that touch board text. This is the
// default IUserRepository (used on Windows always, and on Linux/Docker
// whenever DATABASE_URL isn't set - see server_main.cpp).
class UserRepository : public IUserRepository {
public:
    // dbPath may be a real file path or ":memory:" for an in-process,
    // file-free database (what the unit tests use). Throws
    // std::runtime_error if the database can't be opened or the users
    // table can't be created.
    explicit UserRepository(const std::string& dbPath);
    ~UserRepository() override;

    UserRepository(const UserRepository&) = delete;
    UserRepository& operator=(const UserRepository&) = delete;

    LoginResult login(const std::string& username, const std::string& password) override;
    void updateElo(const std::string& username, int newElo) override;

private:
    sqlite3* db_ = nullptr;
};
