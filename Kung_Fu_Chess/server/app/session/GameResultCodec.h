#pragma once
#include "GameResult.h"
#include "json.hpp"

// Encodes/decodes GameResult for its trip over the EventBus -
// gameEndedTopic is server-internal only (see GameSession::gameEndedTopic)
// but EventBus itself only ever carries nlohmann::json, so this crossing
// still needs a codec even though it never reaches a client. Kept as a
// real Codec class (matching MoveLogCodec/CaptureEventCodec) rather than
// inlined at each call site, since encode() and decode() now live in two
// different files (GameSession.cpp publishes, EloService.cpp consumes)
// that must agree on field names.
class GameResultCodec {
public:
    static nlohmann::json encode(const GameResult& result);
    static GameResult      decode(const nlohmann::json& j);
};
