#pragma once
#include "WsClient.h"
#include "../../shared/protocol/Message.h"
#include "json.hpp"

// Sends `request` over `client` (already connected) and blocks the
// calling thread until a reply tagged "type": <expectedType> arrives,
// returned decoded as json. Any other message received in the meantime
// (a malformed frame, or a reply/push of a different type - e.g. a room's
// first periodic snapshot arriving before the room-join reply itself,
// now that EnterRoom's room-join happens before the reply is sent) is
// silently skipped rather than mistaken for the answer. Takes over
// `client`'s onMessage handler for the duration of the call and does not
// restore whatever handler (if any) was installed before - the caller
// installs its own handler afterward if it needs one.
nlohmann::json sendAndWaitForReply(WsClient& client, const Message& request, MessageType expectedType);
