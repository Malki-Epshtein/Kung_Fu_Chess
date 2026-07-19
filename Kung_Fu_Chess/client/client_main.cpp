#include "client_main.h"
#include "net/WsClient.h"
#include "../shared/protocol/MessageCodec.h"
#include <iostream>

namespace {
    constexpr uint16_t kPort = 9002;
}

int client_main(int /*argc*/, char** /*argv*/) {
    Message hello;
    hello.type = MessageType::Hello;
    std::string encoded = MessageCodec::encode(hello);

    std::cout << "[client] connecting to localhost:" << kPort << std::endl;
    try {
        WsClient client;
        client.run("localhost", kPort, encoded);
    } catch (const std::exception& e) {
        std::cerr << "[client] failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
