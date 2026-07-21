#pragma once
#include "RoomIdentity.h"
#include "SessionRegistry.h"
#include "ClientSessionRegistry.h"
#include <string>

// Joins SessionRegistry (which connection holds which role in a room) with
// ClientSessionRegistry (which connection belongs to which logged-in
// username/elo) into one RoomIdentity - the only place these two
// registries are ever cross-referenced. Deliberately not a method on
// either registry: SessionRegistry doesn't need to know identity exists,
// ClientSessionRegistry doesn't need to know rooms exist - each stays
// focused on its own concern (see RoomIdentity.h's own note on why
// room membership isn't duplicated into ClientSession).
class RoomIdentityResolver {
public:
    static RoomIdentity resolve(const SessionRegistry& sessionRegistry,
                                 const ClientSessionRegistry& clientSessions,
                                 const std::string& roomName);
};
