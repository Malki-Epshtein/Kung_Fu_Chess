#pragma once
#include "json.hpp"

enum class MessageType {
    Hello,
    Move,
    Jump,
    Snapshot
};

struct Message {
    MessageType   type;
    nlohmann::json payload;
};
