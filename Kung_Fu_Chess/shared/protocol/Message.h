#pragma once
#include "json.hpp"

// Every type here except GameFound/MoveLogged/CaptureEvent is a
// client->server request that gets a direct reply. Those three are
// server->client pushes: the server sends them unprompted (a match
// found/timed out, a move just completed, a piece was just captured) -
// never as a reply to a message the client just sent.
enum class MessageType {
    Hello,
    Move,
    Jump,
    Snapshot,
    Login,
    JoinRoom,
    FindGame,
    GameFound,
    MoveLogged,
    CaptureEvent,
    // Claims a seat in a room by token instead of by "this connection
    // already ran LOGIN" - what a client sends over WebSocket right after
    // the HTTP-based login/room flow (API Gateway) instead of JoinRoom,
    // since LOGIN itself never happens on this connection.
    EnterRoom,
    // Establishes this connection's identity from a token (minted by the
    // Gateway's POST /login) without claiming a room seat - what a client
    // sends right after connecting so FindGame (Play) has a
    // ClientSession to read, the same way EnterRoom establishes it for the
    // Room path. See AuthHandler.
    Auth
};

struct Message {
    MessageType   type;
    nlohmann::json payload;
};
