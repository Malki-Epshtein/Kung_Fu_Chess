#include "WsGateway.h"
#include "../shared/bus/NatsClient.h"
#include "../shared/discovery/ShardRegistry.h"
#include "../shared/log/Log.h"
#include <asio/io_context.hpp>
#include <cstdlib>
#include <string>

namespace {
    uint16_t envPort(const char* name, uint16_t fallback) {
        const char* value = std::getenv(name);
        return value ? static_cast<uint16_t>(std::stoi(value)) : fallback;
    }
}

int main() {
    Log::init("ws-gateway");
    const uint16_t listenPort = envPort("GATEWAY_PORT", 8080);

    // Unlike before UPSTREAM_URIS existed, this process now has no way to
    // route anything without NATS - shards are discovered from
    // shard.heartbeat (see ShardRegistry), not read from a static config
    // list. Fail fast rather than start a process that can never route a
    // single connection.
    const char* natsUrl = std::getenv("NATS_URL");
    if (!natsUrl) {
        spdlog::error("ws-gateway: NATS_URL not set, nothing to discover shards with");
        return 1;
    }

    try {
        asio::io_context io;
        NatsClient nats(natsUrl);
        ShardRegistry shards(nats);
        WsGateway wsGateway;
        wsGateway.run(io, listenPort, shards);
        io.run();
    } catch (const std::exception& e) {
        spdlog::error("ws-gateway failed to start: {}", e.what());
        return 1;
    }
    return 0;
}
