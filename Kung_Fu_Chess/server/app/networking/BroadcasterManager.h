#pragma once
#include "NetworkBroadcaster.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class SessionRegistry;
class EventBus;

// Owns one NetworkBroadcaster per room, each subscribed only to that
// room's own topic (GameSession::snapshotTopic(roomName)) and sending only
// to that room's own connections - this is what keeps two rooms' snapshots
// from leaking into each other. Knows nothing about WebSockets/websocketpp
// itself beyond the raw send callback it's given; the concrete
// send-to-socket logic lives in WsServer.
class BroadcasterManager {
public:
    using ConnectionHandle = websocketpp::connection_hdl;
    using ConnectionSender = std::function<void(ConnectionHandle, const std::string&)>;

    // Attaches a broadcaster to every room that already exists in
    // `registry` at construction time (e.g. rooms created before the
    // server started) - rooms created later get theirs via attach(), below.
    BroadcasterManager(EventBus& bus, SessionRegistry& registry, ConnectionSender sender);

    // Creates and stores a NetworkBroadcaster for `roomName`. Safe to call
    // once per room's lifetime (called by CreateRoomHandler/FindGameHandler
    // right after a room is created).
    void attach(const std::string& roomName);

private:
    EventBus&        bus_;
    SessionRegistry&  registry_;
    ConnectionSender  sender_;
    std::unordered_map<std::string, std::unique_ptr<NetworkBroadcaster>> broadcasters_;
};
