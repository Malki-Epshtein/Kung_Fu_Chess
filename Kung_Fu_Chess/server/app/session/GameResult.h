#pragma once
#include "../../../shared/model/Piece.h"
#include <string>

// How a finished game ended - not just "was there a winner", so ELO
// application (or future features: game log, stats) can tell a real
// king-capture apart from a disconnect/auto-resign without GameSession
// exposing raw engine internals to figure it out themselves.
enum class GameEndReason { KingCapture, Disconnect };

// Carries the winner/loser's identity directly, captured at the moment the
// game actually ended - NOT re-derived later from GameSession's identity_
// (which keeps refreshing from SessionRegistry every tick, and a
// disconnected player's connection - and so their username/elo - is gone
// from there well before their auto-resign grace period finishes; see
// GameSession::markDisconnectResign).
struct GameResult {
    Chess::Color  winner;
    GameEndReason reason;
    std::string   winnerUsername;
    int           winnerElo = 0;
    std::string   loserUsername;
    int           loserElo = 0;
};
