#pragma once
#include "NetworkBroadcaster.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SessionRegistry;
class EventBus;

// Owns three NetworkBroadcasters per room - one each for
// GameSession::snapshotTopic, GameSession::moveLogTopic, and
// GameSession::captureTopic - each subscribed only to that room's own
// topic and sending only to that room's own connections. This is what
// keeps one room's data from leaking into another's. Knows nothing about
// WebSockets/websocketpp itself beyond the raw send callback it's given;
// the concrete send-to-socket logic lives in WsServer. The message
// envelope (if any) is decided at the publish site (GameSession), not
// here - this class just forwards whatever text a topic publishes.
class BroadcasterManager {
public:
    using ConnectionHandle = websocketpp::connection_hdl;
    using ConnectionSender = std::function<void(ConnectionHandle, const std::string&)>;

    // Attaches a broadcaster to every room that already exists in
    // `registry` at construction time (e.g. rooms created before the
    // server started) - rooms created later get theirs via attach(), below.
    BroadcasterManager(EventBus& bus, SessionRegistry& registry, ConnectionSender sender);

    // Creates and stores all of `roomName`'s NetworkBroadcasters. Safe to
    // call once per room's lifetime (called by CreateRoomHandler/
    // FindGameHandler right after a room is created).
    void attach(const std::string& roomName);

private:
    EventBus&        bus_;
    SessionRegistry&  registry_;
    ConnectionSender  sender_;
    std::unordered_map<std::string, std::vector<std::unique_ptr<NetworkBroadcaster>>> broadcasters_;
};
