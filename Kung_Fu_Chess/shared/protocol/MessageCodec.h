#pragma once
#include "Message.h"
#include <string>

class MessageCodec {
public:
    static std::string encode(const Message& message);
    static Message     decode(const std::string& text);

    // The canonical MessageType<->wire-name mapping, exposed for callers
    // that need to tag a JSON payload with its type without going through
    // the full Message{type, payload} envelope (e.g. a broadcast or a
    // command-ack reply that keeps its own flat shape) - the alternative
    // would be each such caller hand-rolling its own copy of the name
    // string, which is exactly the "magic string" duplication this exists
    // to prevent.
    static std::string  typeName(MessageType type);
    static MessageType  typeFromName(const std::string& name);
};
