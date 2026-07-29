cmake_minimum_required(VERSION 3.16)
project(KungFuChessServer CXX C)

# This CMakeLists.txt builds ONLY the server (plus the shared/ code it
# depends on) - for Docker/Linux deployment. The client stays on the
# existing MSBuild (.vcxproj) setup: it's a Windows-native GUI app
# (OpenCV window + real Win32 dialogs), which has no business running in
# a headless container. See docs/ or ask why there are two build systems
# in one repo before "fixing" that - it's deliberate, not an oversight.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KFC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Kung_Fu_Chess)
set(THIRDPARTY_DIR ${KFC_DIR}/ThirdParty)

add_executable(Server
    ${KFC_DIR}/server/server_main.cpp
    ${KFC_DIR}/server/net/WsServer.cpp
    ${KFC_DIR}/server/app/session/GameSession.cpp
    ${KFC_DIR}/server/app/session/SessionRegistry.cpp
    ${KFC_DIR}/server/app/session/ClientSessionRegistry.cpp
    ${KFC_DIR}/server/app/session/ConnectionHandler.cpp
    ${KFC_DIR}/server/app/session/RoomIdentityResolver.cpp
    ${KFC_DIR}/server/app/session/GameResultCodec.cpp
    ${KFC_DIR}/server/app/networking/NetworkBroadcaster.cpp
    ${KFC_DIR}/server/app/networking/BroadcasterManager.cpp
    ${KFC_DIR}/server/app/logic/MatchTicketRegistry.cpp
    ${KFC_DIR}/server/app/handlers/AllocateRoomHandler.cpp
    ${KFC_DIR}/server/app/logic/CommandDispatcher.cpp
    ${KFC_DIR}/server/app/logic/StartingBoard.cpp
    ${KFC_DIR}/server/app/logic/EloCalculator.cpp
    ${KFC_DIR}/server/app/logic/EloService.cpp
    ${KFC_DIR}/server/app/logic/GameHistoryService.cpp
    ${KFC_DIR}/server/app/handlers/LoginHandler.cpp
    ${KFC_DIR}/server/app/handlers/JoinRoomHandler.cpp
    ${KFC_DIR}/server/app/handlers/FindGameHandler.cpp
    ${KFC_DIR}/server/app/handlers/MessageDispatcher.cpp
    ${KFC_DIR}/server/app/handlers/MessageRouter.cpp
    ${KFC_DIR}/server/app/handlers/EnterRoomHandler.cpp
    ${KFC_DIR}/server/app/handlers/AuthHandler.cpp
    ${KFC_DIR}/server/concurrency/ThreadPool.cpp
    ${KFC_DIR}/server/db/UserRepository.cpp
    ${KFC_DIR}/server/db/PostgresUserRepository.cpp
    ${KFC_DIR}/shared/db/RedisRoomDirectory.cpp
    ${KFC_DIR}/shared/db/RedisClientSessionStore.cpp
    ${KFC_DIR}/shared/db/RedisReconnectStore.cpp
    ${KFC_DIR}/shared/db/PostgresGameHistoryRepository.cpp
    ${KFC_DIR}/server/engine/GameEngine.cpp
    ${KFC_DIR}/server/engine/ScoreObserver.cpp
    ${KFC_DIR}/server/engine/MoveLogObserver.cpp
    ${KFC_DIR}/server/engine/CaptureEventObserver.cpp
    ${KFC_DIR}/server/app/logic/MoveCommand.cpp
    ${KFC_DIR}/server/app/logic/JumpCommand.cpp
    ${KFC_DIR}/server/io/BoardParser.cpp
    ${KFC_DIR}/server/io/BoardPrinter.cpp
    ${KFC_DIR}/server/realtime/MotionPath.cpp
    ${KFC_DIR}/server/realtime/RealTimeArbiter.cpp
    ${KFC_DIR}/server/realtime/CollisionResolver.cpp
    ${KFC_DIR}/server/realtime/ArrivalResolver.cpp
    ${KFC_DIR}/server/rules/BishopRules.cpp
    ${KFC_DIR}/server/rules/KingRules.cpp
    ${KFC_DIR}/server/rules/KnightRules.cpp
    ${KFC_DIR}/server/rules/PawnRules.cpp
    ${KFC_DIR}/server/rules/PieceRules.cpp
    ${KFC_DIR}/server/rules/QueenRules.cpp
    ${KFC_DIR}/server/rules/RookRules.cpp
    ${KFC_DIR}/server/rules/SlidingPieceRules.cpp
    ${KFC_DIR}/server/rules/rule_engine.cpp
    ${KFC_DIR}/shared/model/Board.cpp
    ${KFC_DIR}/shared/model/Piece.cpp
    ${KFC_DIR}/shared/model/PieceStateMachine.cpp
    ${KFC_DIR}/shared/model/Position.cpp
    ${KFC_DIR}/shared/protocol/MessageCodec.cpp
    ${KFC_DIR}/shared/protocol/MoveLogCodec.cpp
    ${KFC_DIR}/shared/protocol/CaptureEventCodec.cpp
    ${KFC_DIR}/shared/bus/EventBus.cpp
    ${KFC_DIR}/shared/bus/NatsClient.cpp
    ${KFC_DIR}/shared/input/ClickResolver.cpp
    ${KFC_DIR}/shared/protocol/GameSnapshotCodec.cpp
    ${THIRDPARTY_DIR}/sqlite3.c
)

# Two separate gateway processes (Server_Design.md's target architecture:
# "the boundary that matters for this design is API Gateway vs. WS
# Gateway") - previously one fused binary, split apart because they don't
# actually share a runtime dependency: WsGateway is a dumb byte-forwarding
# WebSocket proxy with no Redis access at all, while ApiGateway is the
# REST/HTTP half (login/rooms/history) and needs Redis for the room
# directory + session store. Neither links any of the game/session/rules
# code from the Server target above - they cannot decide game rules even
# by accident because they have no access to any of that code.
# Both gateways are NATS clients now (shared/discovery/ShardRegistry
# subscribes to shard.heartbeat) instead of reading a static UPSTREAM_URIS
# shard list - see ShardRegistry.h for why: it's what lets the game-server
# tier scale by replica count instead of by editing gateway config.
add_executable(WsGateway
    ${KFC_DIR}/gateway/ws_gateway_main.cpp
    ${KFC_DIR}/gateway/WsGateway.cpp
    ${KFC_DIR}/shared/bus/NatsClient.cpp
    ${KFC_DIR}/shared/discovery/ShardRegistry.cpp
)

# Talks to a shard the same way a real client would, over a throwaway
# internal WS connection, reusing LoginHandler unchanged (POST /rooms goes
# through the Game Allocator over NATS instead - see ApiGateway.cpp).
add_executable(ApiGateway
    ${KFC_DIR}/gateway/api_gateway_main.cpp
    ${KFC_DIR}/gateway/ApiGateway.cpp
    ${KFC_DIR}/shared/db/RedisRoomDirectory.cpp
    ${KFC_DIR}/shared/db/RedisClientSessionStore.cpp
    ${KFC_DIR}/shared/db/PostgresGameHistoryRepository.cpp
    ${KFC_DIR}/shared/bus/NatsClient.cpp
    ${KFC_DIR}/shared/discovery/ShardRegistry.cpp
)

# Server_Design.md step 6: Matchmaker and Game Allocator as their own
# services reachable over NATS, split from the shard process the same way
# WsGateway/ApiGateway were split from it in step 4/5 above - see
# shared/matchmaking/MatchTicketRegistry.h for why pairing moved out of
# process. Matchmaker's waiting pool is Redis-backed (RedisMatchPool) so
# 2+ replicas can share it safely instead of each racing its own in-memory
# copy - see RedisMatchPool.h and matchmaker_main.cpp's subscribeQueue
# call. The old in-memory Matchmaker class (shared/matchmaking/Matchmaker.h/
# .cpp) isn't part of this executable at all anymore - matchmaker_main.cpp
# no longer uses it - it's kept only for its own tests
# (test_Matchmaker.cpp, built by the separate Windows/MSBuild test
# project, not this file). GameAllocator also needs Redis, to mint
# globally-unique room names (RedisSequence) and know it exists.
add_executable(Matchmaker
    ${KFC_DIR}/matchmaker/matchmaker_main.cpp
    ${KFC_DIR}/shared/matchmaking/RedisMatchPool.cpp
    ${KFC_DIR}/shared/bus/NatsClient.cpp
)

add_executable(GameAllocator
    ${KFC_DIR}/allocator/allocator_main.cpp
    ${KFC_DIR}/shared/db/RedisSequence.cpp
    ${KFC_DIR}/shared/bus/NatsClient.cpp
)

# sqlite3.c is vendored C (not our code) - same reasoning as the
# SDLCheck/CRT_SECURE_NO_WARNINGS override in Server.vcxproj: it uses a
# couple of deprecated Windows APIs internally on Windows builds only.
set_source_files_properties(${THIRDPARTY_DIR}/sqlite3.c PROPERTIES
    COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS"
)

foreach(tgt Server WsGateway ApiGateway Matchmaker GameAllocator)
    target_include_directories(${tgt} PRIVATE
        ${THIRDPARTY_DIR}
        ${THIRDPARTY_DIR}/include
    )

    target_compile_definitions(${tgt} PRIVATE
        ASIO_STANDALONE
        _WEBSOCKETPP_CPP11_STL_
        # Explicit rather than relying on websocketpp's auto-detection: it
        # has an old guard that skips C++11 threads specifically on MinGW
        # (assuming, no longer true, that MinGW can't do <thread>/<mutex>) -
        # this sidesteps that on every platform/compiler rather than falling
        # back to needing Boost.
        _WEBSOCKETPP_CPP11_THREAD_
    )

    if(WIN32)
        target_compile_definitions(${tgt} PRIVATE _WIN32_WINNT=0x0601)
        target_link_libraries(${tgt} PRIVATE ws2_32 mswsock)
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(${tgt} PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
    endif()
endforeach()

# Postgres client - Server and ApiGateway (WsGateway stays dependency-free
# of it, same as Redis above - a dumb byte-forwarding proxy needs neither).
# This is the one dependency in the repo that isn't vendored under
# ThirdParty/, since libpq isn't header-only or a single amalgamated file
# the way sqlite3/json/asio/websocketpp/spdlog are - it's installed via apt
# in the Dockerfile instead (libpq-dev to build, libpq5 at runtime).
# KFC_HAVE_POSTGRES gates PostgresUserRepository in server_main.cpp and
# PostgresGameHistoryRepository in both server_main.cpp and
# api_gateway_main.cpp - undefined on Windows, where none of those files are
# ever compiled (no .vcxproj references them), keeping the native MSBuild
# build on SQLite (and no game history at all) with zero new dependencies.
find_package(PostgreSQL REQUIRED)
foreach(tgt Server ApiGateway)
    target_link_libraries(${tgt} PRIVATE PostgreSQL::PostgreSQL)
    target_compile_definitions(${tgt} PRIVATE KFC_HAVE_POSTGRES)
endforeach()

# Redis client - Server, ApiGateway, and Matchmaker, not WsGateway (a dumb
# byte-forwarding proxy with no Redis access - see its comment above).
# Server writes the room directory and reads/writes nothing else; ApiGateway
# reads the room directory (JOIN resolution) and writes/reads the session
# store (login tokens) - see ApiGateway.cpp; Matchmaker reads/writes the
# shared waiting pool (RedisMatchPool). Same story as Postgres above:
# hiredis isn't header-only or a single amalgamated file, so it's
# apt-installed in the Dockerfile (libhiredis-dev to build, the matching
# runtime package at runtime) rather than vendored. No CMake built-in find
# module exists for it (unlike PostgreSQL), hence pkg-config.
# KFC_HAVE_REDIS gates every Redis-backed codepath in server_main.cpp -
# undefined on Windows, where that file is compiled without it (no .vcxproj
# defines it); api_gateway_main.cpp/matchmaker_main.cpp have no Windows
# build at all, so neither needs the same guard.
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)
foreach(tgt Server ApiGateway Matchmaker GameAllocator)
    target_include_directories(${tgt} PRIVATE ${HIREDIS_INCLUDE_DIRS})
    target_link_libraries(${tgt} PRIVATE ${HIREDIS_LIBRARIES})
    target_compile_definitions(${tgt} PRIVATE KFC_HAVE_REDIS)
endforeach()

# NATS client (cnats) - Server (optionally, gated by KFC_HAVE_NATS - see
# below), plus WsGateway/ApiGateway/Matchmaker/GameAllocator unconditionally
# (none of the four have a reason to exist without it - see ShardRegistry.h
# for the gateways' side of that). Built from source in the Dockerfile (no
# bookworm apt package, unlike hiredis/libpq) into the default /usr/local
# prefix - no reliable pkg-config or CMake package export across cnats
# versions to depend on, so it's located directly the same way CMake finds
# any manually-installed C library.
find_path(NATS_INCLUDE_DIR nats/nats.h)
find_library(NATS_LIBRARY NAMES nats)
if(NOT NATS_INCLUDE_DIR OR NOT NATS_LIBRARY)
    message(FATAL_ERROR "cnats not found - see Dockerfile for how it's built from source")
endif()
foreach(tgt Server WsGateway ApiGateway Matchmaker GameAllocator)
    target_include_directories(${tgt} PRIVATE ${NATS_INCLUDE_DIR})
    target_link_libraries(${tgt} PRIVATE ${NATS_LIBRARY})
endforeach()
# KFC_HAVE_NATS gates every NATS-backed codepath in server_main.cpp/
# SessionRegistry/etc - undefined on Windows (no .vcxproj defines it),
# where NatsClient.cpp is simply never compiled. The other four targets
# don't need the guard themselves (none are ever built on Windows - no
# .vcxproj references any of gateway/, matchmaker/, or allocator/ - so
# NatsClient.h's declarations are always available to them).
target_compile_definitions(Server PRIVATE KFC_HAVE_NATS)
