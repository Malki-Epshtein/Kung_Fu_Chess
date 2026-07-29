#pragma once
#include "IGameHistoryRepository.h"
#include <mutex>

struct pg_conn;
using PGconn = pg_conn;

// Postgres-backed IGameHistoryRepository - Docker/Linux build only (never
// compiled into the Windows/MSBuild build - not referenced by any .vcxproj,
// same as PostgresUserRepository). Used two ways: a shard constructs one to
// write (server_main.cpp, via GameHistoryService, when DATABASE_URL is set)
// and the API Gateway constructs one to read (api_gateway_main.cpp, same
// env var) for GET /history.
class PostgresGameHistoryRepository : public IGameHistoryRepository {
public:
    // conninfo is a libpq connection string/URI, same shape
    // PostgresUserRepository takes. Throws std::runtime_error if the
    // connection or the games table can't be created.
    explicit PostgresGameHistoryRepository(const std::string& conninfo);
    ~PostgresGameHistoryRepository() override;

    PostgresGameHistoryRepository(const PostgresGameHistoryRepository&) = delete;
    PostgresGameHistoryRepository& operator=(const PostgresGameHistoryRepository&) = delete;

    void record(const GameRecord& record) override;
    std::vector<GameRecord> list(int limit) const override;

private:
    PGconn* conn_ = nullptr;

    // A PGconn isn't thread-safe (same reasoning as PostgresUserRepository's
    // mutex_): record() fires from GameHistoryService::onGameEnded, which
    // runs wherever GameSession::publishStep/markDisconnectResign runs -
    // possibly a ThreadPool worker, same as EloService::onGameEnded.
    mutable std::mutex mutex_;
};
