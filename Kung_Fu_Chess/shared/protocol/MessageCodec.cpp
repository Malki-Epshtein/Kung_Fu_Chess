#include "MessageCodec.h"
#include <stdexcept>
#include <unordered_map>

namespace {
    const std::unordered_map<MessageType, std::string> kTypeToName = {
        { MessageType::Hello,    "HELLO"    },
        { MessageType::Move,     "MOVE"     },
        { MessageType::Jump,     "JUMP"     },
        { MessageType::Snapshot, "SNAPSHOT" },
        { MessageType::Login,    "LOGIN"    },
        { MessageType::JoinRoom,   "JOIN_ROOM"   },
        { MessageType::FindGame,   "FIND_GAME"   },
        { MessageType::GameFound,  "GAME_FOUND"  },
        { MessageType::MoveLogged, "MOVE_LOGGED" },
        { MessageType::CaptureEvent, "CAPTURE_EVENT" },
        { MessageType::EnterRoom, "ENTER_ROOM" },
        { MessageType::Auth,      "AUTH"       },
    };

}

std::string MessageCodec::encode(const Message& message) {
    nlohmann::json j;
    j["type"]    = typeName(message.type);
    j["payload"] = message.payload;
    return j.dump();
}

Message MessageCodec::decode(const std::string& text) {
    nlohmann::json j = nlohmann::json::parse(text);
    Message message;
    message.type    = typeFromName(j.at("type").get<std::string>());
    message.payload = j.value("payload", nlohmann::json::object());
    return message;
}

std::string MessageCodec::typeName(MessageType type) {
    return kTypeToName.at(type);
}

MessageType MessageCodec::typeFromName(const std::string& name) {
    for (const auto& [type, typeName] : kTypeToName)
        if (typeName == name)
            return type;
    throw std::runtime_error("Unknown message type: " + name);
}
