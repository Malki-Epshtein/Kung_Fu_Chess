#pragma once
#include <string>

struct sqlite3;

struct LoginResult {
    bool        success;
    std::string message;
    int         elo = 0; // meaningful only when success is true
};

// The only thing in this codebase that touches SQL - wraps the
// SQLite-backed users table (username, password, elo). CommandDispatcher/
// WsServer never see a query directly, matching how BoardParser/
// BoardPrinter are the only things that touch board text.
class UserRepository {
public:
    // dbPath may be a real file path or ":memory:" for an in-process,
    // file-free database (what the unit tests use). Throws
    // std::runtime_error if the database can't be opened or the users
    // table can't be created.
    explicit UserRepository(const std::string& dbPath);
    ~UserRepository();

    UserRepository(const UserRepository&) = delete;
    UserRepository& operator=(const UserRepository&) = delete;

    // If `username` doesn't exist yet, creates it with `password` and the
    // starting ELO (1200), then logs in. If it exists, succeeds only when
    // `password` matches what's stored for it.
    LoginResult login(const std::string& username, const std::string& password);

    // Overwrites `username`'s stored elo - no-op if that username doesn't
    // exist (shouldn't happen: only ever called with a username just read
    // back out of an active, logged-in ClientSession).
    void updateElo(const std::string& username, int newElo);

private:
    sqlite3* db_ = nullptr;
};
