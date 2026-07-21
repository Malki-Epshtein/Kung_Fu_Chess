#include "MoveLogCodec.h"
#include <stdexcept>
#include <unordered_map>

namespace {
    const std::unordered_map<Chess::Color, std::string> kColorToName = {
        { Chess::Color::White, "White" }, { Chess::Color::Black, "Black" },
    };
    Chess::Color nameToColor(const std::string& name) {
        for (const auto& [color, colorName] : kColorToName)
            if (colorName == name) return color;
        throw std::runtime_error("Unknown color: " + name);
    }

    nlohmann::json encodeEntry(const MoveEntry& entry) {
        return { {"timestamp", entry.timestamp}, {"notation", entry.notation} };
    }
    MoveEntry decodeEntry(const nlohmann::json& j) {
        return MoveEntry{ j.at("timestamp").get<std::string>(), j.at("notation").get<std::string>() };
    }

    nlohmann::json encodeList(const std::vector<MoveEntry>& moves) {
        auto arr = nlohmann::json::array();
        for (const auto& entry : moves)
            arr.push_back(encodeEntry(entry));
        return arr;
    }
    std::vector<MoveEntry> decodeList(const nlohmann::json& j) {
        std::vector<MoveEntry> moves;
        for (const auto& entryJson : j)
            moves.push_back(decodeEntry(entryJson));
        return moves;
    }
}

nlohmann::json MoveLogCodec::encode(Chess::Color color, const MoveEntry& entry) {
    nlohmann::json j = encodeEntry(entry);
    j["color"] = kColorToName.at(color);
    return j;
}

MoveLogEvent MoveLogCodec::decode(const nlohmann::json& j) {
    return MoveLogEvent{ nameToColor(j.at("color").get<std::string>()), decodeEntry(j) };
}

nlohmann::json MoveLogCodec::encodeAll(const std::vector<MoveEntry>& whiteMoves, const std::vector<MoveEntry>& blackMoves) {
    return { {"white", encodeList(whiteMoves)}, {"black", encodeList(blackMoves)} };
}

MoveLogBundle MoveLogCodec::decodeAll(const nlohmann::json& j) {
    return MoveLogBundle{ decodeList(j.at("white")), decodeList(j.at("black")) };
}
