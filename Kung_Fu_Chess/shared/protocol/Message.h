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
    CreateRoom,
    JoinRoom,
    FindGame,
    GameFound,
    MoveLogged,
    CaptureEvent
};

struct Message {
    MessageType   type;
    nlohmann::json payload;
};
