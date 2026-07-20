#include "SessionRegistry.h"
#include <asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <iostream>

namespace {
    constexpr int kDisconnectGraceSeconds = 20;

    const char* roleName(Chess::Color color) {
        switch (color) {
            case Chess::Color::White: return "White";
            case Chess::Color::Black: return "Black";
            default:                  return "Spectator";
        }
    }
}

bool SessionRegistry::createRoom(const std::string& name, std::shared_ptr<Board> board, EventBus& bus, bool simultaneousMode) {
    if (rooms_.count(name))
        return false;

    Room room;
    room.session = std::make_unique<GameSession>(board, bus, name, simultaneousMode);
    rooms_.emplace(name, std::move(room));
    return true;
}

bool SessionRegistry::roomExists(const std::string& name) const {
    return rooms_.count(name) != 0;
}

GameSession* SessionRegistry::room(const std::string& name) {
    auto it = rooms_.find(name);
    return it == rooms_.end() ? nullptr : it->second.session.get();
}

std::vector<std::string> SessionRegistry::roomNames() const {
    std::vector<std::string> names;
    names.reserve(rooms_.size());
    for (const auto& [name, room] : rooms_)
        names.push_back(name);
    return names;
}

bool SessionRegistry::joinRoom(const std::string& name, ConnectionHandle hdl) {
    auto it = rooms_.find(name);
    if (it == rooms_.end())
        return false;

    Room& room = it->second;
    ++room.connectionCount;
    Chess::Color role = room.connectionCount == 1 ? Chess::Color::White
                       : room.connectionCount == 2 ? Chess::Color::Black
                       : Chess::Color::None;
    room.roles[hdl] = role;
    connectionRoom_[hdl] = name;
    return true;
}

Chess::Color SessionRegistry::leave(ConnectionHandle hdl) {
    auto roomIt = connectionRoom_.find(hdl);
    if (roomIt == connectionRoom_.end())
        return Chess::Color::None;

    Chess::Color role = Chess::Color::None;
    auto rIt = rooms_.find(roomIt->second);
    if (rIt != rooms_.end()) {
        auto& roles = rIt->second.roles;
        auto roleIt = roles.find(hdl);
        if (roleIt != roles.end()) {
            role = roleIt->second;
            roles.erase(roleIt);
        }
    }
    connectionRoom_.erase(roomIt);
    return role;
}

std::vector<SessionRegistry::ConnectionHandle> SessionRegistry::connectionsInRoom(const std::string& name) const {
    std::vector<ConnectionHandle> result;
    auto it = rooms_.find(name);
    if (it == rooms_.end())
        return result;
    result.reserve(it->second.roles.size());
    for (const auto& [hdl, role] : it->second.roles)
        result.push_back(hdl);
    return result;
}

const std::string* SessionRegistry::roomOf(ConnectionHandle hdl) const {
    auto it = connectionRoom_.find(hdl);
    return it == connectionRoom_.end() ? nullptr : &it->second;
}

Chess::Color SessionRegistry::roleOf(ConnectionHandle hdl) const {
    auto roomIt = connectionRoom_.find(hdl);
    if (roomIt == connectionRoom_.end())
        return Chess::Color::None;
    auto rIt = rooms_.find(roomIt->second);
    if (rIt == rooms_.end())
        return Chess::Color::None;
    auto roleIt = rIt->second.roles.find(hdl);
    return roleIt == rIt->second.roles.end() ? Chess::Color::None : roleIt->second;
}

void SessionRegistry::startDisconnectCountdown(const std::string& roomName, Chess::Color color, asio::io_context& ioContext) {
    auto timer     = std::make_shared<asio::steady_timer>(ioContext);
    auto remaining = std::make_shared<int>(kDisconnectGraceSeconds);
    auto handler   = std::make_shared<std::function<void(const asio::error_code&)>>();

    *handler = [this, timer, remaining, roomName, color, handler](const asio::error_code& ec) {
        if (ec)
            return;
        GameSession* gameSession = room(roomName);
        if (!gameSession)
            return; // room no longer exists - nothing to update

        gameSession->setDisconnectStatus({ true, color, *remaining });
        if (*remaining <= 0) {
            std::cout << "[server] " << roleName(color) << " auto-resigned (disconnected too long)" << std::endl;
            return;
        }
        std::cout << "[server] " << roleName(color) << " disconnected - auto-resign in " << *remaining << "s" << std::endl;
        --(*remaining);
        timer->expires_after(std::chrono::seconds(1));
        timer->async_wait(*handler);
    };
    timer->expires_after(std::chrono::seconds(0));
    timer->async_wait(*handler);
}
