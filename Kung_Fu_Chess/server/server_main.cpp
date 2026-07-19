#include "server_main.h"
#include "net/WsServer.h"
#include <iostream>

namespace {
    constexpr uint16_t kPort = 9002;
}

int server_main(int /*argc*/, char** /*argv*/) {
    std::cout << "[server] starting on port " << kPort << std::endl;
    try {
        WsServer server;
        server.run(kPort);
    } catch (const std::exception& e) {
        std::cerr << "[server] failed to start: " << e.what()
                   << " (is port " << kPort << " already in use by another instance?)" << std::endl;
        return 1;
    }
    return 0;
}
