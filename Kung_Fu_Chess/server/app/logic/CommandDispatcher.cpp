#include "CommandDispatcher.h"
#include "MoveCommand.h"
#include "JumpCommand.h"
#include "../../../shared/model/Position.h"
#include <exception>

namespace {
    Position positionFromJson(const nlohmann::json& j) {
        return Position{ j.at("row").get<int>(), j.at("col").get<int>() };
    }

    const SnapshotPiece* pieceAt(const GameSnapshot& snapshot, Position pos) {
        for (const auto& p : snapshot.pieces)
            if (p.cell == pos)
                return &p;
        return nullptr;
    }
}

DispatchResult CommandDispatcher::dispatch(const Message& message, GameEngine& engine, Chess::Color senderColor) {
    try {
        switch (message.type) {
            case MessageType::Move: {
                if (senderColor == Chess::Color::None)
                    return { false, "spectators cannot move pieces" };

                Position from = positionFromJson(message.payload.at("from"));
                Position to   = positionFromJson(message.payload.at("to"));

                // Named, not a temporary bound inline: pieceAt() returns a
                // pointer into this snapshot's own pieces vector, which must
                // outlive the pointer's use below.
                GameSnapshot snapshot = engine.snapshot();
                const SnapshotPiece* mover = pieceAt(snapshot, from);
                if (!mover || mover->color != senderColor)
                    return { false, "not your piece" };

                MoveCommand(from, to).execute(engine);
                return { true, "move dispatched" };
            }
            case MessageType::Jump: {
                if (senderColor == Chess::Color::None)
                    return { false, "spectators cannot move pieces" };

                Position pos = positionFromJson(message.payload.at("pos"));

                GameSnapshot snapshot = engine.snapshot();
                const SnapshotPiece* mover = pieceAt(snapshot, pos);
                if (!mover || mover->color != senderColor)
                    return { false, "not your piece" };

                JumpCommand(pos).execute(engine);
                return { true, "jump dispatched" };
            }
            case MessageType::Hello:
                return { true, "hello acknowledged" };
            case MessageType::Snapshot:
                return { false, "clients may not send SNAPSHOT messages" };
            case MessageType::Login:
                // WsServer intercepts LOGIN before it ever reaches here
                // (it's an identity operation, not a GameEngine action) -
                // this case only exists so the switch stays exhaustive.
                return { false, "login must be handled before dispatch" };
            case MessageType::CreateRoom:
            case MessageType::JoinRoom:
                // Same reasoning as LOGIN: WsServer intercepts these before
                // dispatch (they act on the SessionRegistry, not on a
                // specific room's GameEngine).
                return { false, "room messages must be handled before dispatch" };
            case MessageType::FindGame:
                // Same reasoning again: WsServer intercepts this to run
                // matchmaking (it acts on a waiting pool, not a GameEngine).
                return { false, "FindGame must be handled before dispatch" };
            case MessageType::GameFound:
                // Server->client only (see Message.h) - a client should
                // never send this; this case only exists so the switch
                // stays exhaustive.
                return { false, "GameFound is server-to-client only" };
            case MessageType::MoveLogged:
                // Same reasoning as GameFound: server->client push only.
                return { false, "MoveLogged is server-to-client only" };
            case MessageType::CaptureEvent:
                // Same reasoning again: server->client push only.
                return { false, "CaptureEvent is server-to-client only" };
        }
        return { false, "unknown message type" };
    } catch (const std::exception& e) {
        return { false, std::string("malformed payload: ") + e.what() };
    }
}
