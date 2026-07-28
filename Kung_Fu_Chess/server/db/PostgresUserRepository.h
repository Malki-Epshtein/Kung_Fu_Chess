#pragma once
#include "IUserRepository.h"
#include <mutex>
#include <string>

struct pg_conn;
using PGconn = pg_conn;

// Postgres-backed IUserRepository - Docker/Linux build only (selected in
// server_main.cpp when DATABASE_URL is set; never compiled into the
// Windows/MSBuild build - not referenced by any .vcxproj). Same users table
// shape (username, password, elo) and the same hashing as UserRepository;
// this is a backend swap, not a behavior change.
class PostgresUserRepository : public IUserRepository {
public:
    // conninfo is a libpq connection string/URI, e.g.
    // "postgresql://user:pass@host:5432/dbname". Throws std::runtime_error
    // if the connection or the users table can't be created.
    explicit PostgresUserRepository(const std::string& conninfo);
    ~PostgresUserRepository() override;

    PostgresUserRepository(const PostgresUserRepository&) = delete;
    PostgresUserRepository& operator=(const PostgresUserRepository&) = delete;

    LoginResult login(const std::string& username, const std::string& password) override;
    void updateElo(const std::string& username, int newElo) override;

private:
    PGconn* conn_ = nullptr;

    // A PGconn isn't thread-safe, unlike SQLite's default serialized mode:
    // login() runs on the io_service thread, but updateElo() can run from a
    // ThreadPool worker (EloService::onGameEnded fires from inside
    // GameSession::computeStep's tick) - these can overlap across two
    // different rooms even with a single shard.
    std::mutex mutex_;
};
