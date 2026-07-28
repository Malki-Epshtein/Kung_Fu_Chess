#include "PostgresUserRepository.h"
#include <libpq-fe.h>
#include <functional>
#include <stdexcept>

namespace {
    constexpr int kStartingElo = 1200;

    // Same hash as UserRepository - not cryptographically secure, kept
    // identical on purpose (separate backlog item, not touched by this
    // backend swap).
    std::string hashPassword(const std::string& password) {
        return std::to_string(std::hash<std::string>{}(password));
    }
}

PostgresUserRepository::PostgresUserRepository(const std::string& conninfo) {
    conn_ = PQconnectdb(conninfo.c_str());
    if (PQstatus(conn_) != CONNECTION_OK) {
        std::string message = "Cannot connect to Postgres: " + std::string(PQerrorMessage(conn_));
        PQfinish(conn_);
        throw std::runtime_error(message);
    }

    const char* createTableSql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  username TEXT PRIMARY KEY,"
        "  password TEXT NOT NULL,"
        "  elo INTEGER NOT NULL DEFAULT 1200"
        ");";
    PGresult* result = PQexec(conn_, createTableSql);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        // IF NOT EXISTS is a check-then-create, not atomic - two shards
        // starting at once (as happens routinely now that there's more than
        // one) can both pass the check and race on the create. Whichever one
        // loses gets a real Postgres error here, not the harmless NOTICE the
        // sequential case gets - but the end state (table exists) is exactly
        // what we wanted either way, so this specific failure isn't fatal.
        std::string sqlState = PQresultErrorField(result, PG_DIAG_SQLSTATE) ?
            PQresultErrorField(result, PG_DIAG_SQLSTATE) : "";
        bool alreadyExists = sqlState == "42P07" /* duplicate_table */ ||
                             sqlState == "23505" /* unique_violation, e.g. on pg_type's own catalog */;
        if (!alreadyExists) {
            std::string message = "Cannot create users table: " + std::string(PQerrorMessage(conn_));
            PQclear(result);
            PQfinish(conn_);
            throw std::runtime_error(message);
        }
    }
    PQclear(result);
}

PostgresUserRepository::~PostgresUserRepository() {
    PQfinish(conn_);
}

LoginResult PostgresUserRepository::login(const std::string& username, const std::string& password) {
    std::string hashed = hashPassword(password);
    std::lock_guard<std::mutex> lock(mutex_);

    const char* selectParams[1] = { username.c_str() };
    PGresult* select = PQexecParams(conn_,
        "SELECT password, elo FROM users WHERE username = $1;",
        1, nullptr, selectParams, nullptr, nullptr, 0);

    if (PQresultStatus(select) == PGRES_TUPLES_OK && PQntuples(select) > 0) {
        std::string storedHash = PQgetvalue(select, 0, 0);
        int elo = std::stoi(PQgetvalue(select, 0, 1));
        PQclear(select);

        if (storedHash != hashed)
            return { false, "wrong password", 0 };
        return { true, "login successful", elo };
    }
    PQclear(select);

    // No existing account under this username - create one.
    std::string eloStr = std::to_string(kStartingElo);
    const char* insertParams[3] = { username.c_str(), hashed.c_str(), eloStr.c_str() };
    PGresult* insert = PQexecParams(conn_,
        "INSERT INTO users(username, password, elo) VALUES ($1, $2, $3);",
        3, nullptr, insertParams, nullptr, nullptr, 0);
    bool inserted = PQresultStatus(insert) == PGRES_COMMAND_OK;
    PQclear(insert);

    if (!inserted)
        return { false, "could not create account", 0 };
    return { true, "account created", kStartingElo };
}

void PostgresUserRepository::updateElo(const std::string& username, int newElo) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string eloStr = std::to_string(newElo);
    const char* params[2] = { eloStr.c_str(), username.c_str() };
    PGresult* result = PQexecParams(conn_,
        "UPDATE users SET elo = $1 WHERE username = $2;",
        2, nullptr, params, nullptr, nullptr, 0);
    PQclear(result);
}
