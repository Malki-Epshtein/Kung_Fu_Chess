#pragma once
#include <string>

// What the server knows about one connected, logged-in client - keyed by
// connection in ClientSessionRegistry below. Room membership is NOT
// duplicated here: SessionRegistry::roomOf(hdl) is the single source of
// truth for that, to avoid the two ever drifting out of sync.
struct ClientSession {
    std::string username;
    int         elo = 0;
    // Stage H: set while this connection is waiting in the matchmaking
    // pool for FindGame to find it an opponent.
    bool        searchingForGame = false;
};
