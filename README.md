# Kung Fu Chess

A real-time chess variant played over the network: unlike standard chess,
there are no turns — both players can move simultaneously, and each piece
has its own per-move cooldown. Built as a client-server system in C++17,
with a headless WebSocket server and a native Windows GUI client.

## Features

- **Real-time movement** — no turn order; every piece has its own cooldown
  and rest state (`Idle` → `Moving`/`Jump` → `ShortRest`/`LongRest`)
- **Accounts & ELO rating** — shell-based login (username/password,
  SQLite-backed), starting ELO 1200, adjusted relative to opponent rating
  once a game ends
- **Rooms** — create or join a room by a typed name; first occupant plays
  White, second plays Black, everyone after that spectates. Live player
  names/ELO and spectator count are shown for the whole room
- **Matchmaking** — automatic pairing within ±100 ELO, with a 60s search
  timeout if no opponent is found
- **Disconnect handling** — a 20s on-screen grace period before a
  disconnected player auto-resigns
- **Resizable board** — drag the handle at the board's bottom-right corner
  to scale the whole board (pieces, panels, everything) between 60–120px
  per cell
- **Sound effects** — distinct sounds for a click, a move, a capture, an
  illegal move, a jump, and game over, each on its own audio channel so
  overlapping sounds don't cut each other off
- **Concurrent room ticking** — every active room's simulation step runs on
  a fixed-size thread pool (sized to the machine's core count) instead of
  one shared main thread, so a slow room can't stall every other room's
  clients
- **Dual-side logging** — both client and server log every protocol
  message, for debugging a distributed system from either end

## Architecture

One authoritative server holds all game state; every client is a thin
renderer that sends `MOVE`/`JUMP`/`CreateRoom`/`JoinRoom`/`FindGame`
requests and renders whatever snapshot the server broadcasts back. All
game logic (movement legality, collisions, timing, room membership,
matchmaking) lives server-side only.

```
Kung_Fu_Chess/
├── shared/       protocol (Message/MessageCodec), models, EventBus,
│                 click-resolution state machine (shared by the client and
│                 the scripted test harness)
├── server/
│   ├── app/
│   │   ├── session/     ClientSession(Registry), SessionRegistry, GameSession
│   │   ├── networking/  NetworkBroadcaster, BroadcasterManager
│   │   └── logic/       CommandDispatcher, Matchmaker, StartingBoard
│   ├── engine/       GameEngine, ScoreObserver, MoveLogObserver
│   ├── realtime/     RealTimeArbiter, MotionPath, collision/arrival resolution
│   ├── rules/        per-piece movement rules
│   ├── concurrency/  ThreadPool (fixed worker pool driving room ticks)
│   ├── db/           UserRepository (SQLite)
│   └── net/          WsServer (WebSocket handling, message routing)
└── client/
    ├── app/       GraphicalApplication, LoginFlow, HomeScreenView, RoomDialog
    ├── net/       WsClient (dedicated network thread), BlockingRequest,
    │              NetworkMessageHandler (inbound message parsing/state)
    ├── input/     Controller, BoardMapper
    ├── audio/     SoundPlayer (MCI-based, multi-channel)
    └── view/      ImageView (OpenCV rendering), sprites/animation, BoardScale
```

Design patterns in use: **Command** (MoveCommand/JumpCommand), **Registry**
(SessionRegistry, ClientSessionRegistry), **Repository** (UserRepository),
**Pub/Sub** (EventBus — one topic per room, so broadcasts never leak
between rooms), **Facade** (GameEngine), **DTO** (GameSnapshot),
**Thread Pool** (server/concurrency — one shared job queue driving every
room's simulation step, decoupled from network I/O).

## Tech stack

| Concern | Library |
|---|---|
| WebSocket transport | [websocketpp](https://github.com/zaphoyd/websocketpp) + standalone [Asio](https://think-async.com/Asio/) |
| JSON (de)serialization | [nlohmann/json](https://github.com/nlohmann/json) |
| Account storage | SQLite (amalgamation) |
| Client rendering | OpenCV 4.5.1 |
| Unit testing | [doctest](https://github.com/doctest/doctest) |

## Building & running

### Windows (full solution: client + server + tests)

Open `Kung_Fu_Chess.sln` in Visual Studio 2022 and build. This produces
three separate targets:

- **`Server.exe`** (x64) — the headless game server. Run with no arguments:
  ```
  Server.exe
  ```
- **`Client.exe`** (x64) — the graphical client. Must be run from the
  `Kung_Fu_Chess/` project folder (asset paths are relative):
  ```
  Client.exe
  ```
- **`Kung_Fu_Chess.exe`** (Win32/x86) — the unit test suite (doctest).

### Server only, via CMake (e.g. for Docker/Linux)

The client is a Windows-native GUI app (OpenCV window + real Win32
dialogs) and isn't meant to run headless; the server has no such
dependency and builds standalone:

```
cmake -B build
cmake --build build
```

## Testing

416 [doctest](https://github.com/doctest/doctest) unit/integration test
cases across 50+ test files, covering the rule engine, real-time arbiter,
protocol codecs, session/matchmaking logic, and a scripted end-to-end DSL
(`texttests/`) that drives full games through simulated clicks against a
real `GameEngine` and asserts on the resulting board.

Build `Kung_Fu_Chess.exe` as **Win32/x86**, not x64 — the test binary's
`main()` (`test/doctest_main.cpp`) is only compiled into the Win32
configuration.

## Protocol

Messages are JSON envelopes (`{"type": ..., "payload": ...}`) over a
single WebSocket connection per client. Most are client→server requests
with a direct reply; `GAME_FOUND` is the one server-initiated push,
sent whenever a matchmaking search resolves (match or timeout) without
the client having just asked.
