#include "SessionRegistry.h"

bool SessionRegistry::createRoom(const std::string& name, std::shared_ptr<Board> board, EventBus& bus, bool simultaneousMode) {
    if (rooms_.count(name))
        return false;

    Room room;
    room.session = std::make_unique<GameSession>(board, bus, simultaneousMode);
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
