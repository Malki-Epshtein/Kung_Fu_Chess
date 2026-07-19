#include "AnimationConfig.h"
#include "../../../shared/model/PieceStateMachine.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace {
    std::string readWholeFile(const std::string& path) {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("Cannot open config file: " + path);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Pulls the raw value text for "key": <value> out of a flat/known-shape
    // JSON document. Not a general JSON parser - relies on every key in this
    // project's config.json files appearing exactly once.
    std::string extractRawValue(const std::string& json, const std::string& key) {
        std::string needle = "\"" + key + "\"";
        size_t keyPos = json.find(needle);
        if (keyPos == std::string::npos)
            throw std::runtime_error("Missing key '" + key + "' in config");

        size_t colon = json.find(':', keyPos);
        size_t valueStart = json.find_first_not_of(" \t\r\n", colon + 1);

        if (json[valueStart] == '"') {
            size_t valueEnd = json.find('"', valueStart + 1);
            return json.substr(valueStart + 1, valueEnd - valueStart - 1);
        }

        size_t valueEnd = json.find_first_of(",}\r\n", valueStart);
        std::string raw = json.substr(valueStart, valueEnd - valueStart);
        while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())))
            raw.pop_back();
        return raw;
    }
}

AnimationConfig loadAnimationConfig(const std::string& path) {
    std::string json = readWholeFile(path);

    AnimationConfig config;
    config.speed_m_per_sec          = std::stod(extractRawValue(json, "speed_m_per_sec"));
    config.next_state_when_finished = stateFromString(extractRawValue(json, "next_state_when_finished"));
    config.frames_per_sec           = std::stoi(extractRawValue(json, "frames_per_sec"));
    config.frame_count              = std::stoi(extractRawValue(json, "frame_count"));
    config.is_loop                  = extractRawValue(json, "is_loop") == "true";

    return config;
}
