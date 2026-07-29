#include "MatchTicketRegistry.h"

MatchTicketRegistry::MatchTicketRegistry(std::string shardAddress) : shardAddress_(std::move(shardAddress)) {}

std::string MatchTicketRegistry::add(ConnectionHandle hdl) {
    std::string ticketId = shardAddress_ + "#" + std::to_string(nextId_++);
    ticketByConnection_[hdl] = ticketId;
    connectionByTicket_[ticketId] = hdl;
    return ticketId;
}

std::optional<std::string> MatchTicketRegistry::cancel(ConnectionHandle hdl) {
    auto it = ticketByConnection_.find(hdl);
    if (it == ticketByConnection_.end())
        return std::nullopt;
    std::string ticketId = it->second;
    connectionByTicket_.erase(ticketId);
    ticketByConnection_.erase(it);
    return ticketId;
}

std::optional<MatchTicketRegistry::ConnectionHandle> MatchTicketRegistry::resolve(const std::string& ticketId) {
    auto it = connectionByTicket_.find(ticketId);
    if (it == connectionByTicket_.end())
        return std::nullopt;
    ConnectionHandle hdl = it->second;
    connectionByTicket_.erase(it);
    ticketByConnection_.erase(hdl);
    return hdl;
}
