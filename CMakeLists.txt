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
    ${KFC_DIR}/server/app/networking/NetworkBroadcaster.cpp
    ${KFC_DIR}/server/app/logic/Matchmaker.cpp
    ${KFC_DIR}/server/app/logic/CommandDispatcher.cpp
    ${KFC_DIR}/server/app/logic/StartingBoard.cpp
    ${KFC_DIR}/server/db/UserRepository.cpp
    ${KFC_DIR}/server/engine/GameEngine.cpp
    ${KFC_DIR}/server/engine/ScoreObserver.cpp
    ${KFC_DIR}/server/engine/MoveLogObserver.cpp
    ${KFC_DIR}/server/input/MoveCommand.cpp
    ${KFC_DIR}/server/input/JumpCommand.cpp
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
    ${KFC_DIR}/shared/bus/EventBus.cpp
    ${KFC_DIR}/shared/input/ClickResolver.cpp
    ${KFC_DIR}/shared/protocol/GameSnapshotCodec.cpp
    ${THIRDPARTY_DIR}/sqlite3.c
)

target_include_directories(Server PRIVATE
    ${THIRDPARTY_DIR}
    ${THIRDPARTY_DIR}/include
)

target_compile_definitions(Server PRIVATE
    ASIO_STANDALONE
    _WEBSOCKETPP_CPP11_STL_
    # Explicit rather than relying on websocketpp's auto-detection: it
    # has an old guard that skips C++11 threads specifically on MinGW
    # (assuming, no longer true, that MinGW can't do <thread>/<mutex>) -
    # this sidesteps that on every platform/compiler rather than falling
    # back to needing Boost.
    _WEBSOCKETPP_CPP11_THREAD_
)

# sqlite3.c is vendored C (not our code) - same reasoning as the
# SDLCheck/CRT_SECURE_NO_WARNINGS override in Server.vcxproj: it uses a
# couple of deprecated Windows APIs internally on Windows builds only.
set_source_files_properties(${THIRDPARTY_DIR}/sqlite3.c PROPERTIES
    COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS"
)

if(WIN32)
    target_compile_definitions(Server PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(Server PRIVATE ws2_32 mswsock)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(Server PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()
