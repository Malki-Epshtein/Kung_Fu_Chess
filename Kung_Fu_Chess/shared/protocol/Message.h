#pragma once
#include "json.hpp"

// Every type here except GameFound is a client->server request that gets
// a direct reply. GameFound (Stage H) is the first server->client push:
// the server sends it unprompted, whenever a match is actually found or a
// FindGame search times out - never as a reply to a message the client
// just sent.
enum class MessageType {
    Hello,
    Move,
    Jump,
    Snapshot,
    Login,
    CreateRoom,
    JoinRoom,
    FindGame,
    GameFound
};

struct Message {
    MessageType   type;
    nlohmann::json payload;
};
