#pragma once
#include <cstdint>

// Minimal WebSocket server: logs connects/disconnects, and on every message
// decodes it (to log the type) and echoes the raw text back. No game logic
// yet - that arrives in Stage B (CommandDispatcher).
class WsServer {
public:
    void run(uint16_t port);
};
