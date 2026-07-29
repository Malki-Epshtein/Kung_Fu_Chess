#pragma once
#include "IMessageHandler.h"
#include "../../../shared/bus/INatsClient.h"
#include <asio/io_context.hpp>
#include <functional>
#include <string>

class ClientSessionRegistry;
class MatchTicketRegistry;

// On a FindGame request, mints a ticket (MatchTicketRegistry) and
// publishes it to the out-of-process Matchmaker service instead of pairing
// in-process (Server_Design.md step 6 - see MatchTicketRegistry's header
// for the full reasoning). The eventual pairing result arrives
// asynchronously over NATS, on subjects every shard subscribes to at
// construction time - handle() itself only ever sends the request and
// returns the immediate "searching" ack; matchmaking.assigned/.timeout are
// handled by the subscriptions below, since they may resolve a ticket
// belonging to a DIFFERENT call to handle() than the one currently running
// (possibly even on a different shard, for the ticket that WASN'T waiting
// here).
class FindGameHandler : public IMessageHandler {
public:
    using PushToConnection = std::function<void(ConnectionHandle, const std::string&)>;

    // `nats` may be null (native Windows build, or a Docker build with no
    // NATS_URL) - FIND_GAME simply fails gracefully in that case, same
    // degrade-without-crashing pattern as EnterRoomHandler/AuthHandler's
    // sessionStore.
    FindGameHandler(ClientSessionRegistry& sessions, MatchTicketRegistry& tickets,
                     INatsClient* nats, PushToConnection push, asio::io_context& ioContext);

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    // NATS subscription callbacks run on cnats' own delivery thread, not
    // the io_service thread - these two do the actual MatchTicketRegistry/
    // push work, always invoked via ioContext_.post() from the subscribe
    // lambdas in the constructor, never directly from that foreign thread
    // (same reasoning as WsServer.cpp's tickPool worker threads posting
    // back before touching shared state).
    void handleAssigned(const nlohmann::json& event);
    void handleTimeout(const nlohmann::json& event);

    ClientSessionRegistry& sessions_;
    MatchTicketRegistry&    tickets_;
    INatsClient*             nats_;
    PushToConnection         push_;
    asio::io_context&        ioContext_;
};
