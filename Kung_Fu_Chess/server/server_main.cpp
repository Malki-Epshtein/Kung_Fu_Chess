#include "server_main.h"
#include "net/WsServer.h"
#include "app/SessionRegistry.h"
#include "app/StartingBoard.h"
#include "db/UserRepository.h"
#include "../shared/bus/EventBus.h"
#include <iostream>

namespace {
    constexpr uint16_t kPort = 9002;
    constexpr const char* kUserDbPath = "users.db";
}

int server_main(int /*argc*/, char** /*argv*/) {
    std::cout << "[server] starting on port " << kPort << std::endl;
    try {
        EventBus bus;
        SessionRegistry registry;
        registry.createRoom(WsServer::kDefaultRoomName, makeStartingBoard(), bus, /*simultaneousMode=*/true);
        UserRepository users(kUserDbPath);
        WsServer server;
        server.run(kPort, registry, bus, users);
    } catch (const std::exception& e) {
        std::cerr << "[server] failed to start: " << e.what()
                   << " (is port " << kPort << " already in use by another instance?)" << std::endl;
        return 1;
    }
    return 0;
}
