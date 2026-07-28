#pragma once
#include <asio/io_context.hpp>
#include <cstdint>
#include <string>
#include <vector>

// A dumb byte-forwarding proxy: terminates the real client's WebSocket and
// opens one upstream WebSocket to a game-server shard per client, then
// shuttles text frames between the two verbatim. It never parses a
// message, never touches game rules, and never decides anything - the game
// server remains the sole authority, exactly as it is today when a client
// connects to it directly. This is what lets the game server's existing
// connection_hdl-keyed registries (SessionRegistry, ClientSessionRegistry,
// Matchmaker) work completely unmodified: from a shard's point of view, the
// Gateway's upstream connection *is* the client.
//
// Which shard: if the client's WebSocket handshake carries an
// "X-Shard-Hint" header (set by the client after the API Gateway's REST
// room resolution told it which shard actually hosts the room - see
// ApiGateway), that shard is used directly. Otherwise (no room chosen yet,
// or the header is missing/unrecognized - e.g. a client using the old
// direct-WS flow), round-robin across `shardUris`, same as before this
// existed.
//
// Runs on the `io` passed in rather than owning one - ApiGateway shares the
// same io_context so gateway_main.cpp can run both off a single io.run().
class WsGateway {
public:
    void run(asio::io_context& io, uint16_t listenPort, const std::vector<std::string>& shardUris);
};
