#pragma once
#include <string>

struct LoginResult {
    bool        success;
    std::string message;
    int         elo = 0; // meaningful only when success is true
};

// Common surface for whatever actually stores the users table - today that's
// UserRepository (SQLite, used everywhere on Windows and as the default
// fallback) and PostgresUserRepository (Docker/Linux build only, selected at
// runtime via DATABASE_URL - see server_main.cpp). CommandDispatcher/WsServer
// never see a query directly regardless of which one is behind this.
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    // If `username` doesn't exist yet, creates it with `password` and the
    // starting ELO (1200), then logs in. If it exists, succeeds only when
    // `password` matches what's stored for it.
    virtual LoginResult login(const std::string& username, const std::string& password) = 0;

    // Overwrites `username`'s stored elo - no-op if that username doesn't
    // exist (shouldn't happen: only ever called with a username just read
    // back out of an active, logged-in ClientSession).
    virtual void updateElo(const std::string& username, int newElo) = 0;
};
