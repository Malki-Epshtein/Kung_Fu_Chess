#pragma once
#include "../net/WsClient.h"
#include <iostream>
#include <string>

// Result of the shell-based login handshake (see runLoginFlow below).
struct LoginResult {
    bool        success = false;
    std::string message;
    int         elo = 0;
};

// Blocks the calling thread: waits for `client`'s connection to open, then
// prompts `in`/`out` (the shell) for a username/password, sends a LOGIN
// message over `client`, and waits for the server's reply. Retries up to
// 3 attempts on failure (e.g. wrong password) before giving up. Must run
// before the graphical window opens, and before any other onMessage
// handler is installed on `client` - it takes over the connection's
// message handler for the duration of the login handshake and does not
// restore a prior one.
LoginResult runLoginFlow(WsClient& client, std::istream& in = std::cin, std::ostream& out = std::cout);
