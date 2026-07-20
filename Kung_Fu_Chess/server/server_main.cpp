#include "server_main.h"
#include "net/WsServer.h"
#include "app/SessionRegistry.h"
#include "io/BoardParser.h"
#include "../shared/bus/EventBus.h"
#include <iostream>
#include <sstream>

namespace {
    constexpr uint16_t kPort = 9002;

    std::shared_ptr<Board> startingBoard() {
        std::istringstream boardText(
            "Board:\n"
            "bR bN bB bQ bK bB bN bR\n"
            "bP bP bP bP bP bP bP bP\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            "wP wP wP wP wP wP wP wP\n"
            "wR wN wB wQ wK wB wN wR\n"
            "Commands:\n"
        );
        return BoardParser::parseBoardOnly(boardText);
    }
}

int server_main(int /*argc*/, char** /*argv*/) {
    std::cout << "[server] starting on port " << kPort << std::endl;
    try {
        EventBus bus;
        SessionRegistry registry;
        registry.createRoom(WsServer::kDefaultRoomName, startingBoard(), bus, /*simultaneousMode=*/true);
        WsServer server;
        server.run(kPort, registry, bus);
    } catch (const std::exception& e) {
        std::cerr << "[server] failed to start: " << e.what()
                   << " (is port " << kPort << " already in use by another instance?)" << std::endl;
        return 1;
    }
    return 0;
}
