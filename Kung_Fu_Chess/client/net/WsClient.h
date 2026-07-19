#pragma once
#include <cstdint>
#include <string>

// Minimal WebSocket client: connects to the server, sends one message, logs
// whatever comes back. No game logic yet - see WsServer.h.
class WsClient {
public:
    void run(const std::string& host, uint16_t port, const std::string& messageToSend);
};
