#pragma once
#include "WsClient.h"
#include "../../shared/protocol/Message.h"
#include "json.hpp"

// Sends `request` over `client` (already connected) and blocks the
// calling thread until exactly one reply arrives, returned decoded as
// json. Takes over `client`'s onMessage handler for the duration of the
// call and does not restore whatever handler (if any) was installed
// before - the caller installs its own handler afterward if it needs one.
// Shared by the login handshake and the Room dialog's CreateRoom/JoinRoom
// exchange, which both need this exact "send one request, block for one
// direct reply" shape.
nlohmann::json sendAndWaitForReply(WsClient& client, const Message& request);
