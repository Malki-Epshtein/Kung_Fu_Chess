#include "MessageRouter.h"

void MessageRouter::registerHandler(MessageType type, IMessageHandler& handler) {
    handlers_[type] = &handler;
}

IMessageHandler* MessageRouter::find(MessageType type) {
    auto it = handlers_.find(type);
    return it != handlers_.end() ? it->second : nullptr;
}
