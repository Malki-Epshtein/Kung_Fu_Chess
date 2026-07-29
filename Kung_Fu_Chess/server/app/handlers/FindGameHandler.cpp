#include "FindGameHandler.h"
#include "../session/ClientSessionRegistry.h"
#include "../logic/MatchTicketRegistry.h"
#include "../../../shared/protocol/Message.h"
#include "../../../shared/protocol/MessageCodec.h"
#include "../../../shared/protocol/MoveLogCodec.h"
#include "../../../shared/log/Log.h"

FindGameHandler::FindGameHandler(ClientSessionRegistry& sessions, MatchTicketRegistry& tickets,
                                  INatsClient* nats, PushToConnection push, asio::io_context& ioContext)
    : sessions_(sessions), tickets_(tickets), nats_(nats), push_(std::move(push)), ioContext_(ioContext) {
    if (!nats_)
        return;

    // Fires once per matched pair, delivered to every shard subscribed -
    // each just no-ops for the ticket it doesn't recognize (see
    // MatchTicketRegistry::resolve). Whichever shard the Game Allocator
    // actually put the room on is carried in the event itself ("shard"),
    // not implied by who receives this.
    nats_->subscribe("matchmaking.assigned", [this](const nlohmann::json& event) {
        ioContext_.post([this, event] { handleAssigned(event); });
    });
    nats_->subscribe("matchmaking.timeout", [this](const nlohmann::json& event) {
        ioContext_.post([this, event] { handleTimeout(event); });
    });
}

nlohmann::json FindGameHandler::handle(ConnectionHandle hdl, const nlohmann::json& /*payload*/) {
    if (!nats_)
        return { {"success", false}, {"message", "matchmaking unavailable"} };

    const ClientSession* session = sessions_.sessionFor(hdl);
    if (!session)
        return { {"success", false}, {"message", "must be logged in to find a game"} };

    std::string ticketId = tickets_.add(hdl);
    nats_->publish("matchmaking.request",
                    { {"ticketId", ticketId}, {"username", session->username}, {"elo", session->elo} });
    spdlog::info("'{}' searching for opponent (ticket {})", session->username, ticketId);

    return { {"success", true}, {"message", "searching for opponent"} };
}

void FindGameHandler::handleAssigned(const nlohmann::json& event) {
    for (const char* side : {"A", "B"}) {
        std::string ticketId = event.at(std::string("ticket") + side).get<std::string>();
        std::optional<ConnectionHandle> hdl = tickets_.resolve(ticketId);
        if (!hdl)
            continue; // not a ticket this shard owns - the other matched player's, or already handled

        Message msg{ MessageType::GameFound,
            { {"success", true}, {"message", "match found"},
              {"roomName", event.at("roomName")}, {"shard", event.at("shard")},
              {"role", event.at(std::string("role") + side)},
              // Always empty - a freshly allocated room has no history yet
              // (same as the old in-process FindGameHandler's moveLog,
              // which was always registry_.room(name)->fullMoveLog() on a
              // room it had just created itself). Must be the
              // {"white":[...],"black":[...]} shape MoveLogCodec::encodeAll
              // produces - a bare [] fails MoveLogCodec::decodeAll's
              // j.at("white")/j.at("black") on the client with
              // "cannot use at() with array".
              {"moveLog", MoveLogCodec::encodeAll({}, {})},
              {"whiteName", event.at("whiteName")}, {"whiteElo", event.at("whiteElo")},
              {"blackName", event.at("blackName")}, {"blackElo", event.at("blackElo")} } };
        push_(*hdl, MessageCodec::encode(msg));
        spdlog::info("matched, assigned to room '{}' on {}", event.at("roomName").get<std::string>(),
                     event.at("shard").get<std::string>());
    }
}

void FindGameHandler::handleTimeout(const nlohmann::json& event) {
    std::string ticketId = event.at("ticketId").get<std::string>();
    std::optional<ConnectionHandle> hdl = tickets_.resolve(ticketId);
    if (!hdl)
        return;
    Message msg{ MessageType::GameFound, { {"success", false}, {"message", "no players available"} } };
    push_(*hdl, MessageCodec::encode(msg));
    spdlog::info("FindGame timed out, no match found");
}
