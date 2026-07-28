#include "ApiGateway.h"
#include "../shared/log/Log.h"
#include "../shared/db/RedisRoomDirectory.h"
#include "../shared/db/RedisClientSessionStore.h"
#include <asio/io_context.hpp>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {
    uint16_t envPort(const char* name, uint16_t fallback) {
        const char* value = std::getenv(name);
        return value ? static_cast<uint16_t>(std::stoi(value)) : fallback;
    }

    std::string envString(const char* name, const std::string& fallback) {
        const char* value = std::getenv(name);
        return value ? std::string(value) : fallback;
    }

    // "ws://a:9002,ws://b:9002" -> the two URIs - which shard(s) POST
    // /login and POST /rooms relay to (see ApiGateway.cpp's relayToShard).
    std::vector<std::string> splitUris(const std::string& csv) {
        std::vector<std::string> uris;
        std::stringstream ss(csv);
        std::string uri;
        while (std::getline(ss, uri, ','))
            if (!uri.empty())
                uris.push_back(uri);
        return uris;
    }
}

int main() {
    Log::init("api-gateway");
    const uint16_t apiPort = envPort("API_PORT", 8081);
    const std::vector<std::string> shardUris = splitUris(envString("UPSTREAM_URIS", "ws://localhost:9002"));

    // Unlike WsGateway, this process has no reason to exist without Redis -
    // its entire job is relaying to shards and reading/writing the
    // Redis-backed room directory/session store. Fail fast instead of
    // starting a process that can never serve a single route.
    const char* host = std::getenv("REDIS_HOST");
    if (!host) {
        spdlog::error("api-gateway: REDIS_HOST not set, nothing to serve without it");
        return 1;
    }
    const char* portEnv = std::getenv("REDIS_PORT");
    int redisPort = portEnv ? std::atoi(portEnv) : 6379;

    try {
        asio::io_context io;
        RedisRoomDirectory      directory(host, redisPort);
        RedisClientSessionStore sessionStore(host, redisPort);

        ApiGateway apiGateway;
        apiGateway.run(io, apiPort, shardUris, directory, sessionStore);
        io.run();
    } catch (const std::exception& e) {
        spdlog::error("api-gateway failed to start: {}", e.what());
        return 1;
    }
    return 0;
}
