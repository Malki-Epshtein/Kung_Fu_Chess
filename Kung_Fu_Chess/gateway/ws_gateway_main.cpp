#include "WsGateway.h"
#include "../shared/log/Log.h"
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

    // "ws://a:9002,ws://b:9002" -> the two URIs. A single entry (no comma)
    // works the same as the old single-shard UPSTREAM_URI did.
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
    Log::init("ws-gateway");
    const uint16_t listenPort = envPort("GATEWAY_PORT", 8080);
    const std::vector<std::string> shardUris = splitUris(envString("UPSTREAM_URIS", "ws://localhost:9002"));

    try {
        asio::io_context io;
        WsGateway wsGateway;
        wsGateway.run(io, listenPort, shardUris);
        io.run();
    } catch (const std::exception& e) {
        spdlog::error("ws-gateway failed to start: {}", e.what());
        return 1;
    }
    return 0;
}
