#pragma once
#include "json.hpp"

enum class MessageType {
    Hello,
    Move,
    Jump,
    Snapshot,
    Login,
    CreateRoom,
    JoinRoom
};

struct Message {
    MessageType   type;
    nlohmann::json payload;
};
