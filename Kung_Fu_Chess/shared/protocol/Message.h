#pragma once
#include "json.hpp"

// Every type here except GameFound/MoveLogged is a client->server request
// that gets a direct reply. Those two are server->client pushes: the
// server sends them unprompted (a match found/timed out, or a move just
// completed) - never as a reply to a message the client just sent.
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
    MoveLogged
};

struct Message {
    MessageType   type;
    nlohmann::json payload;
};
