#include "PostgresGameHistoryRepository.h"
#include <libpq-fe.h>
#include <stdexcept>

PostgresGameHistoryRepository::PostgresGameHistoryRepository(const std::string& conninfo) {
    conn_ = PQconnectdb(conninfo.c_str());
    if (PQstatus(conn_) != CONNECTION_OK) {
        std::string message = "Cannot connect to Postgres: " + std::string(PQerrorMessage(conn_));
        PQfinish(conn_);
        throw std::runtime_error(message);
    }

    const char* createTableSql =
        "CREATE TABLE IF NOT EXISTS games ("
        "  id SERIAL PRIMARY KEY,"
        "  room_name TEXT NOT NULL,"
        "  winner_username TEXT NOT NULL,"
        "  winner_elo INTEGER NOT NULL,"
        "  loser_username TEXT NOT NULL,"
        "  loser_elo INTEGER NOT NULL,"
        "  reason TEXT NOT NULL,"
        "  move_log JSONB NOT NULL,"
        "  played_at TIMESTAMPTZ NOT NULL DEFAULT now()"
        ");";
    PGresult* result = PQexec(conn_, createTableSql);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        // Same IF NOT EXISTS check-then-create race as
        // PostgresUserRepository's users table - harmless when two shards
        // (or a shard and the API Gateway) start against a fresh Postgres
        // at once, the end state (table exists) is what we wanted either way.
        std::string sqlState = PQresultErrorField(result, PG_DIAG_SQLSTATE) ?
            PQresultErrorField(result, PG_DIAG_SQLSTATE) : "";
        bool alreadyExists = sqlState == "42P07" /* duplicate_table */ ||
                             sqlState == "23505" /* unique_violation, e.g. on pg_type's own catalog */;
        if (!alreadyExists) {
            std::string message = "Cannot create games table: " + std::string(PQerrorMessage(conn_));
            PQclear(result);
            PQfinish(conn_);
            throw std::runtime_error(message);
        }
    }
    PQclear(result);
}

PostgresGameHistoryRepository::~PostgresGameHistoryRepository() {
    PQfinish(conn_);
}

void PostgresGameHistoryRepository::record(const GameRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string winnerEloStr = std::to_string(record.winnerElo);
    std::string loserEloStr  = std::to_string(record.loserElo);
    std::string moveLogStr   = record.moveLog.dump();
    const char* params[7] = {
        record.roomName.c_str(), record.winnerUsername.c_str(), winnerEloStr.c_str(),
        record.loserUsername.c_str(), loserEloStr.c_str(), record.reason.c_str(), moveLogStr.c_str(),
    };
    // paramTypes left null (same as PostgresUserRepository's inserts) -
    // Postgres infers each param's type from the column it's being
    // inserted into, including the text->jsonb cast for move_log.
    PGresult* result = PQexecParams(conn_,
        "INSERT INTO games(room_name, winner_username, winner_elo, loser_username, loser_elo, reason, move_log) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7);",
        7, nullptr, params, nullptr, nullptr, 0);
    PQclear(result);
}

std::vector<GameRecord> PostgresGameHistoryRepository::list(int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string limitStr = std::to_string(limit);
    const char* params[1] = { limitStr.c_str() };
    PGresult* result = PQexecParams(conn_,
        "SELECT room_name, winner_username, winner_elo, loser_username, loser_elo, reason, move_log "
        "FROM games ORDER BY played_at DESC LIMIT $1;",
        1, nullptr, params, nullptr, nullptr, 0);

    std::vector<GameRecord> records;
    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        int rows = PQntuples(result);
        records.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            GameRecord record;
            record.roomName       = PQgetvalue(result, i, 0);
            record.winnerUsername = PQgetvalue(result, i, 1);
            record.winnerElo      = std::stoi(PQgetvalue(result, i, 2));
            record.loserUsername  = PQgetvalue(result, i, 3);
            record.loserElo       = std::stoi(PQgetvalue(result, i, 4));
            record.reason         = PQgetvalue(result, i, 5);
            try {
                record.moveLog = nlohmann::json::parse(PQgetvalue(result, i, 6));
            } catch (const std::exception&) {
                record.moveLog = nlohmann::json::object();
            }
            records.push_back(std::move(record));
        }
    }
    PQclear(result);
    return records;
}
