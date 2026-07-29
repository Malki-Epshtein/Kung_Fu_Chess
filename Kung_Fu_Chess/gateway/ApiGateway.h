#pragma once
#include "../shared/db/IRoomDirectory.h"
#include "../shared/db/IClientSessionStore.h"
#include "../shared/db/IGameHistoryRepository.h"
#include "../shared/discovery/ShardRegistry.h"
#include "../shared/bus/INatsClient.h"
#include <asio/io_context.hpp>
#include <cstdint>
#include <string>

// The REST/HTTP half of the Gateway (see WsGateway for the WebSocket half)
// - implements the diagram's "API Gateway: login, rooms, history" box,
// with Auth Service and Rooms API as its two sub-responsibilities (per
// Server_Design.md: "they can start as functions inside the one API
// Gateway process rather than three separate binaries").
//
// POST /login talks to a game-server shard the same way a real client
// would - a plain internal WebSocket connection sending the exact same
// LOGIN message a client sends - so none of LoginHandler's server-side
// logic needs duplicating here. It never validates a password or decides
// game rules itself; it only relays and remembers a token.
//
// POST /rooms instead asks the Game Allocator (over NATS) to pick a
// least-loaded shard and create the room there - the same mechanism a PLAY
// match already uses (see AllocateRoomHandler) - rather than this process
// round-robining over shards itself.
//
// POST /play mints a matchmaking ticket, publishes it to the Matchmaker
// service over NATS, and holds the HTTP response open until
// matchmaking.assigned/.timeout arrives for that ticket - the diagram's
// "matchmaking requests" over REST, replacing the old direct WebSocket
// FIND_GAME the client used to send straight to a shard.
//
// Routes:
//   POST /login              {username,password} -> {success,message,elo,token}
//   POST /rooms               {token,name}         -> {success,message,roomName,shard}
//   POST /rooms/{name}/join   {token}              -> {success,message,roomName,shard}
//   POST /play                {token}              -> {success,message,roomName,shard}
//   GET  /rooms                                    -> {success,rooms:[...]}
//   GET  /history                                  -> {success,games:[...]}
//
// Runs on the `io` passed in - shares it with WsGateway so gateway_main.cpp
// can drive both off a single io.run().
class ApiGateway {
public:
    // `gameHistory` is optional (null: GET /history always returns an empty
    // list rather than failing the whole process - unlike REDIS_HOST/
    // NATS_URL, nothing else here depends on it existing).
    void run(asio::io_context& io, uint16_t listenPort, ShardRegistry& shards,
             IRoomDirectory& directory, IClientSessionStore& sessionStore, INatsClient& nats,
             IGameHistoryRepository* gameHistory = nullptr);
};
