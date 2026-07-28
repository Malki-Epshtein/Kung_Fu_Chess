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
    ${KFC_DIR}/server/app/logic/Matchmaker.cpp
    ${KFC_DIR}/server/app/logic/CommandDispatcher.cpp
    ${KFC_DIR}/server/app/logic/StartingBoard.cpp
    ${KFC_DIR}/server/app/logic/EloCalculator.cpp
    ${KFC_DIR}/server/app/logic/EloService.cpp
    ${KFC_DIR}/server/app/handlers/LoginHandler.cpp
    ${KFC_DIR}/server/app/handlers/CreateRoomHandler.cpp
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
add_executable(WsGateway
    ${KFC_DIR}/gateway/ws_gateway_main.cpp
    ${KFC_DIR}/gateway/WsGateway.cpp
)

# Talks to a shard the same way a real client would, over a throwaway
# internal WS connection, reusing LoginHandler/CreateRoomHandler unchanged.
add_executable(ApiGateway
    ${KFC_DIR}/gateway/api_gateway_main.cpp
    ${KFC_DIR}/gateway/ApiGateway.cpp
    ${KFC_DIR}/shared/db/RedisRoomDirectory.cpp
    ${KFC_DIR}/shared/db/RedisClientSessionStore.cpp
)

# sqlite3.c is vendored C (not our code) - same reasoning as the
# SDLCheck/CRT_SECURE_NO_WARNINGS override in Server.vcxproj: it uses a
# couple of deprecated Windows APIs internally on Windows builds only.
set_source_files_properties(${THIRDPARTY_DIR}/sqlite3.c PROPERTIES
    COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS"
)

foreach(tgt Server WsGateway ApiGateway)
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

# Postgres client - Server only, neither gateway (both stay dependency-free
# of it on purpose, see their comment above). This is the one dependency in the repo
# that isn't vendored under ThirdParty/, since libpq isn't header-only or a
# single amalgamated file the way sqlite3/json/asio/websocketpp/spdlog are -
# it's installed via apt in the Dockerfile instead (libpq-dev to build,
# libpq5 at runtime). KFC_HAVE_POSTGRES gates the PostgresUserRepository
# codepath in server_main.cpp - undefined on Windows, where this file is
# simply never compiled (no .vcxproj references it), keeping the native
# MSBuild build on SQLite with zero new dependencies.
find_package(PostgreSQL REQUIRED)
target_link_libraries(Server PRIVATE PostgreSQL::PostgreSQL)
target_compile_definitions(Server PRIVATE KFC_HAVE_POSTGRES)

# Redis client - Server and ApiGateway, not WsGateway (a dumb byte-forwarding
# proxy with no Redis access - see its comment above). Server writes the room
# directory and reads/writes nothing else; ApiGateway reads the room directory
# (JOIN resolution) and writes/reads the session store (login tokens) - see
# ApiGateway.cpp. Same story as Postgres above: hiredis isn't header-only or
# a single amalgamated file, so it's apt-installed in the Dockerfile
# (libhiredis-dev to build, the matching runtime package at runtime) rather
# than vendored. No CMake built-in find module exists for it (unlike
# PostgreSQL), hence pkg-config. KFC_HAVE_REDIS gates every Redis-backed
# codepath in server_main.cpp - undefined on Windows, where that file is
# compiled without it (no .vcxproj defines it); api_gateway_main.cpp has no
# Windows build at all, so it doesn't need the same guard.
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)
foreach(tgt Server ApiGateway)
    target_include_directories(${tgt} PRIVATE ${HIREDIS_INCLUDE_DIRS})
    target_link_libraries(${tgt} PRIVATE ${HIREDIS_LIBRARIES})
    target_compile_definitions(${tgt} PRIVATE KFC_HAVE_REDIS)
endforeach()
