#include "PieceStateMachine.h"
#include <vector>
#include <utility>
#include <stdexcept>

namespace {
    const std::vector<std::pair<Chess::State, Chess::State>> kLegalTransitions = {
        { Chess::State::Idle,      Chess::State::Moving },
        { Chess::State::Idle,      Chess::State::Jump },
        { Chess::State::Moving,    Chess::State::LongRest },
        { Chess::State::Jump,      Chess::State::ShortRest },
        { Chess::State::ShortRest, Chess::State::Idle },
        { Chess::State::LongRest,  Chess::State::Idle },
    };
}

//אם המעבר חוקי
bool isLegalTransition(Chess::State from, Chess::State to) {
    if (to == Chess::State::Captured)
        return true;

    for (const auto& transition : kLegalTransitions)
        if (transition.first == from && transition.second == to)
            return true;

    return false;
}
// הםא הכלי עסוק
bool isBusyState(Chess::State state) {
    return state == Chess::State::Moving || state == Chess::State::Jump;
}

//אם הכלי במנוחה
bool isRestingState(Chess::State state) {
    return state == Chess::State::ShortRest || state == Chess::State::LongRest;
}

Chess::State stateFromString(const std::string& name) {
    if (name == "idle")       return Chess::State::Idle;
    if (name == "move")       return Chess::State::Moving;
    if (name == "jump")       return Chess::State::Jump;
    if (name == "short_rest") return Chess::State::ShortRest;
    if (name == "long_rest")  return Chess::State::LongRest;
    throw std::invalid_argument("Unknown state name in config: " + name);
}

std::string stateToFolderName(Chess::State state) {
    switch (state) {
        case Chess::State::Idle:      return "idle";
        case Chess::State::Moving:    return "move";
        case Chess::State::Jump:      return "jump";
        case Chess::State::ShortRest: return "short_rest";
        case Chess::State::LongRest:  return "long_rest";
        default: throw std::invalid_argument("No sprite folder for this state");
    }
}
