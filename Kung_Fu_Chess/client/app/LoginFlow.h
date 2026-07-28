#pragma once
#include "../net/HttpClient.h"
#include <iostream>
#include <string>

// Result of the shell-based login handshake (see runLoginFlow below).
struct LoginResult {
    bool        success = false;
    std::string message;
    int         elo = 0;
    // Minted by the Gateway's POST /login (see gateway/ApiGateway.h) -
    // handed to every later REST call (POST /rooms, POST /rooms/{name}/join)
    // and to the WebSocket-side AUTH/EnterRoom messages, since LOGIN itself
    // no longer runs on any WebSocket connection.
    std::string token;
};

// Blocks the calling thread: prompts `in`/`out` (the shell) for a
// username/password and POSTs them to `api`'s /login route. Retries up to
// 3 attempts on failure (e.g. wrong password) before giving up. Purely
// HTTP - unlike the old WS-based handshake, this needs no live WebSocket
// connection at all, so it can run before any WsClient exists.
LoginResult runLoginFlow(HttpClient& api, std::istream& in = std::cin, std::ostream& out = std::cout);
