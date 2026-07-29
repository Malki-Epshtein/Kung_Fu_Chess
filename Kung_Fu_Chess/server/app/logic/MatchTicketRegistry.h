#pragma once
#include <websocketpp/common/connection_hdl.hpp>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

// Shard-local: which locally-connected client is waiting on which
// matchmaking ticket. Replaces the old in-process Matchmaker's pool now
// that pairing itself runs in a separate service (shared/matchmaking/
// Matchmaker) reachable only by portable ticket ID, not a process-local
// connection_hdl - this class is the seam between the two: FindGameHandler
// mints a ticket here and publishes it to that service, then later
// resolves the service's answer back to the right local connection.
// Not thread-safe by design - like SessionRegistry/ClientSessionRegistry,
// every method is only ever called from the WsServer's single io_service
// thread (see FindGameHandler's NATS subscriptions, which marshal onto it
// via io_context::post before touching this).
class MatchTicketRegistry {
public:
    using ConnectionHandle = websocketpp::connection_hdl;

    // Ticket IDs are prefixed with this shard's own address so they stay
    // globally unique across every shard's concurrently-open tickets, even
    // though each shard numbers its own tickets independently (same
    // reasoning FindGameHandler's old nextMatchId_ needed for room names).
    explicit MatchTicketRegistry(std::string shardAddress);

    // Mints a new ticket ID for `hdl` and remembers it.
    std::string add(ConnectionHandle hdl);

    // Removes hdl's ticket (if any) and returns its ID - used when a
    // FindGame search is abandoned by disconnect.
    std::optional<std::string> cancel(ConnectionHandle hdl);

    // Looks up and removes (consumes) the connection waiting on `ticketId` -
    // nullopt if it's not a ticket this shard owns (e.g. it belongs to the
    // OTHER matched player, connected to a different shard).
    std::optional<ConnectionHandle> resolve(const std::string& ticketId);

private:
    std::string shardAddress_;
    int nextId_ = 1;
    std::map<ConnectionHandle, std::string, std::owner_less<ConnectionHandle>> ticketByConnection_;
    std::unordered_map<std::string, ConnectionHandle> connectionByTicket_;
};
