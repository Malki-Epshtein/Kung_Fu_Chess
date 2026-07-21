#include "FindGameHandler.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../session/ClientSessionRegistry.h"
#include "../session/RoomIdentityResolver.h"
#include "../session/RoleName.h"
#include "../logic/Matchmaker.h"
#include "../logic/StartingBoard.h"
#include "../../../shared/protocol/Message.h"
#include "../../../shared/protocol/MessageCodec.h"
#include <asio/steady_timer.hpp>
#include <chrono>
#include <iostream>
#include <memory>

nlohmann::json FindGameHandler::handle(ConnectionHandle hdl, const nlohmann::json& /*payload*/) {
    const ClientSession* session = sessions_.sessionFor(hdl);
    if (!session)
        return { {"success", false}, {"message", "must be logged in to find a game"} };

    auto opponent = matchmaker_.findMatch(session->elo);
    if (opponent) {
        matchmaker_.remove(*opponent);
        std::string name = "match-" + std::to_string(nextMatchId_++);
        registry_.createRoom(name, makeStartingBoard(), bus_, /*simultaneousMode=*/true);
        attachBroadcaster_(name);//פה קורה לי כל ההתחברות
        registry_.joinRoom(name, *opponent); // was waiting first -> White
        registry_.joinRoom(name, hdl);        // just matched -> Black

        // Always empty here (the room was just created for this match) -
        // included for shape consistency with JoinRoom's reply, same as
        // CreateRoom (see GameSession::fullMoveLog).
        nlohmann::json moveLog = registry_.room(name)->fullMoveLog();

        RoomIdentity identity = RoomIdentityResolver::resolve(registry_, sessions_, name);

        // The opponent isn't expecting a reply right now - this is a
        // server->client PUSH (see Message.h), sent unprompted outside the
        // normal reply flow (which only ever reaches `hdl`, the sender).
        Message opponentMsg{ MessageType::GameFound,
            { {"success", true}, {"message", "match found"}, {"roomName", name}, {"role", roleName(registry_.roleOf(*opponent))}, {"moveLog", moveLog},
              {"whiteName", identity.whiteUsername}, {"whiteElo", identity.whiteElo},
              {"blackName", identity.blackUsername}, {"blackElo", identity.blackElo} } };
        push_(*opponent, MessageCodec::encode(opponentMsg));

        // The sender DOES get the normal reply flow (handled by whoever
        // calls this), but shaped as the same GameFound envelope so both
        // players receive an identical message shape.
        Message senderMsg{ MessageType::GameFound,
            { {"success", true}, {"message", "match found"}, {"roomName", name}, {"role", roleName(registry_.roleOf(hdl))}, {"moveLog", moveLog},
              {"whiteName", identity.whiteUsername}, {"whiteElo", identity.whiteElo},
              {"blackName", identity.blackUsername}, {"blackElo", identity.blackElo} } };
        std::cout << "[server] matched '" << name << "' via FindGame" << std::endl;
        return nlohmann::json::parse(MessageCodec::encode(senderMsg));
    }

    matchmaker_.addToPool(hdl, session->elo);

    // 60s search timeout - same asio::steady_timer pattern as the
    // disconnect countdown. isWaiting() tells a real timeout apart from
    // "already matched by someone else's FindGame in the meantime, this
    // firing is now moot".
    Matchmaker&      matchmaker = matchmaker_;
    PushToConnection push       = push_;
    auto timer = std::make_shared<asio::steady_timer>(ioContext_);
    timer->expires_after(std::chrono::seconds(60));
    timer->async_wait([&matchmaker, push, hdl, timer](const asio::error_code& ec) {
        if (ec || !matchmaker.isWaiting(hdl))
            return;
        matchmaker.remove(hdl);
        Message timeoutMsg{ MessageType::GameFound, { {"success", false}, {"message", "no players available"} } };
        push(hdl, MessageCodec::encode(timeoutMsg));
        std::cout << "[server] FindGame timed out, no match found" << std::endl;
    });

    return { {"success", true}, {"message", "searching for opponent"} };
}
