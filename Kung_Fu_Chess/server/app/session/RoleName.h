#pragma once
#include "../../../shared/model/Piece.h"

// "White"/"Black"/"Spectator" for logging and protocol reply fields - the
// single place this mapping lives, shared by WsServer.cpp and every
// message handler that needs to report a connection's role.
inline const char* roleName(Chess::Color color) {
    switch (color) {
        case Chess::Color::White: return "White";
        case Chess::Color::Black: return "Black";
        default:                  return "Spectator";
    }
}
