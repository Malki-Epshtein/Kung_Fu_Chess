#include "net/WsServer.h"
#include "app/session/SessionRegistry.h"
#include "db/UserRepository.h"
#include "../shared/bus/EventBus.h"
#include "../shared/log/Log.h"

namespace {
    constexpr uint16_t kPort = 9002;
    constexpr const char* kUserDbPath = "users.db";
}

int main(int /*argc*/, char** /*argv*/) {
    Log::init("server");
    spdlog::info("starting on port {}", kPort);
    try {
        EventBus bus;
        SessionRegistry registry;
        UserRepository users(kUserDbPath);
        WsServer server;
        server.run(kPort, registry, bus, users);
    } catch (const std::exception& e) {
        spdlog::error("failed to start: {} (is port {} already in use by another instance?)", e.what(), kPort);
        return 1;
    }
    return 0;
}
