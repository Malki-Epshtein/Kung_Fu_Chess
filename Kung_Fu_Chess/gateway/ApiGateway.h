#pragma once
#include "../shared/db/IRoomDirectory.h"
#include "../shared/db/IClientSessionStore.h"
#include <asio/io_context.hpp>
#include <cstdint>
#include <string>
#include <vector>

// The REST/HTTP half of the Gateway (see WsGateway for the WebSocket half)
// - implements the diagram's "API Gateway: login, rooms, history" box,
// with Auth Service and Rooms API as its two sub-responsibilities (per
// Server_Design.md: "they can start as functions inside the one API
// Gateway process rather than three separate binaries").
//
// Talks to a game-server shard the same way a real client would - a plain
// internal WebSocket connection sending the exact same LOGIN/CREATE_ROOM
// messages a client sends - so none of LoginHandler/CreateRoomHandler's
// server-side logic needs duplicating here. It never validates a password
// or decides game rules itself; it only relays and remembers a token.
//
// Routes:
//   POST /login              {username,password} -> {success,message,elo,token}
//   POST /rooms               {token,name}         -> {success,message,roomName,shard}
//   POST /rooms/{name}/join   {token}              -> {success,message,roomName,shard}
//   GET  /rooms                                    -> {success,rooms:[...]}
//   GET  /history                                  -> {success,games:[]} (stub - see plan)
//
// Runs on the `io` passed in - shares it with WsGateway so gateway_main.cpp
// can drive both off a single io.run().
class ApiGateway {
public:
    void run(asio::io_context& io, uint16_t listenPort, const std::vector<std::string>& shardUris,
             IRoomDirectory& directory, IClientSessionStore& sessionStore);
};
