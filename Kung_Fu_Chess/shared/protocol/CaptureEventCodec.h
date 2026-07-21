#pragma once
#include "CaptureEvent.h"
#include "json.hpp"

// Wire format for a single CaptureEvent - sent as the payload of a
// CAPTURE_EVENT push, mirroring MoveLogCodec's shape/role for MOVE_LOGGED.
class CaptureEventCodec {
public:
    static nlohmann::json encode(const CaptureEvent& event);
    static CaptureEvent   decode(const nlohmann::json& j);
};
