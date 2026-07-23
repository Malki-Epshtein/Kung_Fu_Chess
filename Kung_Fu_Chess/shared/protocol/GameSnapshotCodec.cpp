#include "GameSnapshotCodec.h"
#include <stdexcept>
#include <unordered_map>

namespace {
    const std::unordered_map<Chess::Kind, std::string> kKindToName = {
        { Chess::Kind::King,   "King"   }, { Chess::Kind::Queen,  "Queen"  },
        { Chess::Kind::Rook,   "Rook"   }, { Chess::Kind::Bishop, "Bishop" },
        { Chess::Kind::Knight, "Knight" }, { Chess::Kind::Pawn,   "Pawn"   },
        { Chess::Kind::None,   "None"   },
    };
    Chess::Kind nameToKind(const std::string& name) {
        for (const auto& [kind, kindName] : kKindToName)
            if (kindName == name) return kind;
        throw std::runtime_error("Unknown piece kind: " + name);
    }

    const std::unordered_map<Chess::Color, std::string> kColorToName = {
        { Chess::Color::White, "White" }, { Chess::Color::Black, "Black" }, { Chess::Color::None, "None" },
    };
    Chess::Color nameToColor(const std::string& name) {
        for (const auto& [color, colorName] : kColorToName)
            if (colorName == name) return color;
        throw std::runtime_error("Unknown color: " + name);
    }

    const std::unordered_map<Chess::State, std::string> kStateToName = {
        { Chess::State::Idle,      "Idle"      }, { Chess::State::Moving,    "Moving"    },
        { Chess::State::Jump,      "Jump"      }, { Chess::State::ShortRest, "ShortRest" },
        { Chess::State::LongRest,  "LongRest"  }, { Chess::State::Captured,  "Captured"  },
    };
    Chess::State nameToState(const std::string& name) {
        for (const auto& [state, stateName] : kStateToName)
            if (stateName == name) return state;
        throw std::runtime_error("Unknown state: " + name);
    }

    nlohmann::json encodePosition(const Position& p) {
        return { {"row", p.row}, {"col", p.col} };
    }
    Position decodePosition(const nlohmann::json& j) {
        return Position{ j.at("row").get<int>(), j.at("col").get<int>() };
    }
}

nlohmann::json GameSnapshotCodec::encode(const GameSnapshot& snapshot) {
    auto pieces = nlohmann::json::array();
    for (const auto& p : snapshot.pieces) {
        pieces.push_back({
            {"id",                   p.id},
            {"kind",                 kKindToName.at(p.kind)},
            {"color",                kColorToName.at(p.color)},
            {"cell",                 encodePosition(p.cell)},
            {"state",                kStateToName.at(p.state)},
            {"elapsed_in_state_ms",  p.elapsed_in_state_ms},
            {"targetCell",           encodePosition(p.targetCell)},
            {"travelProgress",       p.travelProgress},
            {"restProgress",         p.restProgress},
        });
    }

    auto legalMoves = nlohmann::json::array();
    for (const auto& pos : snapshot.legalMoves)
        legalMoves.push_back(encodePosition(pos));

    return {
        {"board_width",   snapshot.board_width},
        {"board_height",  snapshot.board_height},
        {"pieces",        pieces},
        {"game_over",     snapshot.game_over},
        {"has_selection", snapshot.has_selection},
        {"selected_cell", encodePosition(snapshot.selected_cell)},
        {"legalMoves",    legalMoves},
    };
}

GameSnapshot GameSnapshotCodec::decode(const nlohmann::json& j) {
    GameSnapshot snapshot;
    snapshot.board_width  = j.at("board_width").get<int>();
    snapshot.board_height = j.at("board_height").get<int>();

    for (const auto& pj : j.at("pieces")) {
        SnapshotPiece p;
        p.id                    = pj.at("id").get<int>();
        p.kind                = nameToKind(pj.at("kind").get<std::string>());
        p.color                = nameToColor(pj.at("color").get<std::string>());
        p.cell                  = decodePosition(pj.at("cell"));
        p.state                = nameToState(pj.at("state").get<std::string>());
        p.elapsed_in_state_ms  = pj.at("elapsed_in_state_ms").get<int>();
        p.targetCell            = decodePosition(pj.at("targetCell"));
        p.travelProgress       = pj.at("travelProgress").get<double>();
        p.restProgress         = pj.at("restProgress").get<double>();
        snapshot.pieces.push_back(p);
    }

    snapshot.game_over     = j.at("game_over").get<bool>();
    snapshot.has_selection = j.at("has_selection").get<bool>();
    snapshot.selected_cell = decodePosition(j.at("selected_cell"));

    for (const auto& pos : j.at("legalMoves"))
        snapshot.legalMoves.push_back(decodePosition(pos));

    return snapshot;
}
