#include "CaptureEventCodec.h"
#include <stdexcept>
#include <unordered_map>

namespace {
    const std::unordered_map<Chess::Kind, std::string> kKindToName = {
        { Chess::Kind::King,   "King"   }, { Chess::Kind::Queen,  "Queen"  },
        { Chess::Kind::Rook,   "Rook"   }, { Chess::Kind::Bishop, "Bishop" },
        { Chess::Kind::Knight, "Knight" }, { Chess::Kind::Pawn,   "Pawn"   },
    };
    Chess::Kind nameToKind(const std::string& name) {
        for (const auto& [kind, kindName] : kKindToName)
            if (kindName == name) return kind;
        throw std::runtime_error("Unknown piece kind: " + name);
    }

    const std::unordered_map<Chess::Color, std::string> kColorToName = {
        { Chess::Color::White, "White" }, { Chess::Color::Black, "Black" },
    };
    Chess::Color nameToColor(const std::string& name) {
        for (const auto& [color, colorName] : kColorToName)
            if (colorName == name) return color;
        throw std::runtime_error("Unknown color: " + name);
    }

    nlohmann::json encodePosition(const Position& p) {
        return { {"row", p.row}, {"col", p.col} };
    }
    Position decodePosition(const nlohmann::json& j) {
        return Position{ j.at("row").get<int>(), j.at("col").get<int>() };
    }
}

nlohmann::json CaptureEventCodec::encode(const CaptureEvent& event) {
    return {
        {"kind", kKindToName.at(event.kind)},
        {"color", kColorToName.at(event.color)},
        {"cell", encodePosition(event.cell)},
    };
}

CaptureEvent CaptureEventCodec::decode(const nlohmann::json& j) {
    return CaptureEvent{
        nameToKind(j.at("kind").get<std::string>()),
        nameToColor(j.at("color").get<std::string>()),
        decodePosition(j.at("cell")),
    };
}
