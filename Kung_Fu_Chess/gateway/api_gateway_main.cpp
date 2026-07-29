#include "ApiGateway.h"
#include "../shared/log/Log.h"
#include "../shared/db/RedisRoomDirectory.h"
#include "../shared/db/RedisClientSessionStore.h"
#include "../shared/db/PostgresGameHistoryRepository.h"
#include "../shared/bus/NatsClient.h"
#include "../shared/discovery/ShardRegistry.h"
#include <asio/io_context.hpp>
#include <cstdlib>
#include <memory>
#include <string>

namespace {
    uint16_t envPort(const char* name, uint16_t fallback) {
        const char* value = std::getenv(name);
        return value ? static_cast<uint16_t>(std::stoi(value)) : fallback;
    }
}

int main() {
    Log::init("api-gateway");
    const uint16_t apiPort = envPort("API_PORT", 8081);

    // This process has no reason to exist without Redis - its entire job
    // is relaying to shards and reading/writing the Redis-backed room
    // directory/session store. Fail fast instead of starting a process
    // that can never serve a single route.
    const char* host = std::getenv("REDIS_HOST");
    if (!host) {
        spdlog::error("api-gateway: REDIS_HOST not set, nothing to serve without it");
        return 1;
    }
    const char* portEnv = std::getenv("REDIS_PORT");
    int redisPort = portEnv ? std::atoi(portEnv) : 6379;

    // Likewise has no way to pick a shard to relay to without NATS anymore
    // - shards are discovered from shard.heartbeat (see ShardRegistry),
    // not read from a static UPSTREAM_URIS list.
    const char* natsUrl = std::getenv("NATS_URL");
    if (!natsUrl) {
        spdlog::error("api-gateway: NATS_URL not set, nothing to discover shards with");
        return 1;
    }

    // Optional, unlike REDIS_HOST/NATS_URL above - GET /history just
    // returns an empty list without it (see ApiGateway.cpp), not a reason
    // to refuse every other route.
    std::unique_ptr<PostgresGameHistoryRepository> gameHistory;
    if (const char* databaseUrl = std::getenv("DATABASE_URL")) {
        spdlog::info("connecting to Postgres");
        gameHistory = std::make_unique<PostgresGameHistoryRepository>(databaseUrl);
    }

    try {
        asio::io_context io;
        RedisRoomDirectory      directory(host, redisPort);
        RedisClientSessionStore sessionStore(host, redisPort);
        NatsClient   nats(natsUrl);
        ShardRegistry shards(nats);

        ApiGateway apiGateway;
        apiGateway.run(io, apiPort, shards, directory, sessionStore, nats, gameHistory.get());
        io.run();
    } catch (const std::exception& e) {
        spdlog::error("api-gateway failed to start: {}", e.what());
        return 1;
    }
    return 0;
}
