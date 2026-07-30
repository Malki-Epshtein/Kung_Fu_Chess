# Kung Fu Chess

A real-time chess variant played over the network: unlike standard chess,
there are no turns — both players can move simultaneously, and each piece
has its own per-move cooldown. Built in C++17 as a distributed system: a
horizontally-scalable game-server tier behind a REST/WebSocket gateway
pair, with NATS, Redis, and Postgres as the coordination/persistence tier,
deployable via Docker Compose or Kubernetes.

## Features

- **Real-time movement** — no turn order; every piece has its own cooldown
  and rest state (`Idle` → `Moving`/`Jump` → `ShortRest`/`LongRest`)
- **Accounts & ELO rating** — username/password login, starting ELO 1200,
  adjusted relative to opponent rating once a game ends (SQLite locally,
  Postgres in the distributed deployment)
- **Rooms** — create or join a room by a typed name; first occupant plays
  White, second plays Black, everyone after that spectates. Live player
  names/ELO and spectator count are shown for the whole room
- **Matchmaking** — automatic pairing within ±100 ELO, with a 60s search
  timeout if no opponent is found; runs as its own horizontally-scaled
  service, backed by a shared Redis waiting pool
- **Reconnect within the grace period** — a disconnected player has 20s to
  reconnect (same room, same seat) before auto-resigning; reconnecting in
  time cancels the countdown and the game just continues. The countdown's
  live status also mirrors to Redis for the duration of a disconnect
- **Horizontal scaling** — the game-server tier runs as N stateless-ish
  shard replicas; a Game Allocator picks the least-loaded shard for every
  new room (regular or matched), and clients are routed to the shard that
  actually owns their room
- **Crash forensics** — every in-progress room's current board state and
  move log mirror to Redis on each completed move, so if a shard crashes
  there's still a record of what was happening (inspection only, not
  automatic resume)
- **Observability** — every service exposes `GET /health` and
  `GET /metrics` (Prometheus text format) on its own port
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
- **Dual-side logging** — both client and every server-side process log
  every protocol message/event, for debugging a distributed system from
  any node

## Architecture

One authoritative `GameServer` shard holds the state for any given room;
every client is a thin renderer that sends `MOVE`/`JUMP` (and friends) and
renders whatever snapshot its shard broadcasts back. All game logic
(movement legality, collisions, timing, room membership) lives
server-side only. Everything outside the game engine itself — login,
room/match routing, matchmaking, presence, history — is split into
separate services talking over NATS (events/RPC) and Redis/Postgres
(shared state), so the game-server tier can scale by replica count alone.

```
                         ┌───────────────┐
        REST             │  API Gateway  │  login / rooms / play / history
  Client ───────────────▶ │ (N replicas) │  /health  /metrics
                         └───────┬───────┘
                                 │ NATS (allocation.request, matchmaking.*)
                         ┌───────┴───────┐        ┌──────────────┐
        WebSocket        │  WS Gateway   │        │  Matchmaker  │◀──┐
  Client ───────────────▶│ (N replicas)  │        │ (N replicas) │   │ NATS
                         └───────┬───────┘        └──────┬───────┘   │ queue
                                 │ proxies to the         │ groups
                                 │ shard that owns  ┌──────┴───────┐  │
                                 │ the room         │ GameAllocator│◀─┘
                                 ▼                  │ (N replicas) │
                         ┌───────────────┐          └──────┬───────┘
                         │  GameServer   │◀─────────────────┘
                         │ (N replicas,  │  shard.<addr>.allocate (RPC)
                         │  "shards")    │  shard.heartbeat (load reporting)
                         └───────┬───────┘
                                 │
                  ┌──────────────┼──────────────┐
                  ▼              ▼              ▼
              Postgres         Redis           NATS
          users, finished   room directory,   event bus:
          game history      sessions,         shard.heartbeat,
                             matchmaking       matchmaking.*,
                             pool, reconnect   allocation.request,
                             status, live      room-scoped topics
                             game-state mirror
```

```
Kung_Fu_Chess/
├── shared/         protocol (Message/MessageCodec), models, EventBus
│   ├── bus/        NatsClient/INatsClient, SubjectSafe (shard-address
│   │                sanitizing, shared by every NATS producer/consumer)
│   ├── db/         Redis/Postgres-backed interfaces + implementations:
│   │                room directory, client sessions, reconnect status,
│   │                live game-state mirror, game history
│   ├── discovery/   ShardRegistry (live shard set, from shard.heartbeat)
│   ├── matchmaking/ RedisMatchPool (shared waiting pool)
│   └── metrics/     PrometheusText (hand-rolled text exposition format)
├── server/          the game-server shard binary
│   ├── app/
│   │   ├── session/     ClientSession(Registry), SessionRegistry, GameSession
│   │   ├── networking/  NetworkBroadcaster, BroadcasterManager
│   │   └── logic/       CommandDispatcher, EloService, GameHistoryService,
│   │                    GameStateMirrorService
│   ├── engine/       GameEngine, ScoreObserver, MoveLogObserver
│   ├── realtime/     RealTimeArbiter, MotionPath, collision/arrival resolution
│   ├── rules/        per-piece movement rules
│   ├── concurrency/  ThreadPool (fixed worker pool driving room ticks)
│   ├── db/           UserRepository (SQLite) / PostgresUserRepository
│   └── net/          WsServer (WebSocket handling, message routing)
├── gateway/         ApiGateway (REST) and WsGateway (WS proxy) - two
│                     separate binaries, split since they share no runtime
│                     dependency
├── matchmaker/       Matchmaker service (ELO-range pairing over NATS)
├── allocator/        GameAllocator service (least-loaded shard picking)
└── client/
    ├── app/       GraphicalApplication, LoginFlow, HomeScreenView, RoomDialog
    ├── net/       WsClient (dedicated network thread), HttpClient,
    │              BlockingRequest, NetworkMessageHandler
    ├── input/     Controller, BoardMapper
    ├── audio/     SoundPlayer (MCI-based, multi-channel)
    └── view/      ImageView (OpenCV rendering), sprites/animation, BoardScale
```

Design patterns in use: **Command** (MoveCommand/JumpCommand), **Registry**
(SessionRegistry, ClientSessionRegistry), **Repository**
(UserRepository/PostgresUserRepository, game history, reconnect store,
game-state store), **Pub/Sub** (in-process `EventBus` — one topic per
room; NATS — the same idea, network-reachable, for cross-service events),
**Facade** (GameEngine), **DTO** (GameSnapshot), **Thread Pool**
(server/concurrency — one shared job queue driving every room's
simulation step, decoupled from network I/O).

## Tech stack

| Concern | Library |
|---|---|
| WebSocket transport | [websocketpp](https://github.com/zaphoyd/websocketpp) + standalone [Asio](https://think-async.com/Asio/) |
| JSON (de)serialization | [nlohmann/json](https://github.com/nlohmann/json) |
| Cross-service messaging | [NATS](https://nats.io) ([nats.c](https://github.com/nats-io/nats.c)) — pub/sub, queue groups, request/reply |
| Shared/ephemeral state | [Redis](https://redis.io) ([hiredis](https://github.com/redis/hiredis)) |
| Durable storage | SQLite (amalgamation, native Windows build) / [PostgreSQL](https://www.postgresql.org) (`libpq`, Docker/Linux build) |
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

The native build is single-shard: no NATS/Redis/Postgres, no gateways,
no horizontal scaling. `Server.exe` falls back to SQLite and pure
in-memory state whenever `DATABASE_URL`/`REDIS_HOST`/`NATS_URL` aren't
set — which they never are on Windows — so this is the same game logic,
just running as the one process the whole distributed system decomposes
from.

### Full distributed stack, via Docker Compose

```
docker compose up -d
```

Builds and starts all 5 application services (`gameserver` ×2,
`ws-gateway`, `api-gateway`, `matchmaker` ×2, `game-allocator` ×2) plus
`postgres`/`redis`/`nats`. The client (`GraphicalApplication.h`) connects
to `ws://localhost:9002` (WS Gateway) and `http://localhost:8081`
(API Gateway) with no changes — both host ports match what a native,
single-shard `Server.exe` would have used.

The client only needs Windows/OpenCV; none of the CMake-built services
do. `CMakeLists.txt` builds only `Server`/`WsGateway`/`ApiGateway`/
`Matchmaker`/`GameAllocator` — no client code, no OpenCV dependency:

```
cmake -B build
cmake --build build
```

### Kubernetes

Manifests for every tier live in `k8s/` (one Deployment + Service pair
each) — see `k8s/README.md` for the build/import/apply steps and known
limitations. Targets a local K3s-style cluster, not a production one.

## Testing

432 [doctest](https://github.com/doctest/doctest) unit/integration test
cases across 53 test files, covering the rule engine, real-time arbiter,
protocol codecs, session/matchmaking logic, and a scripted end-to-end DSL
(`texttests/`) that drives full games through simulated clicks against a
real `GameEngine` and asserts on the resulting board. The
NATS/Redis/Postgres-backed services (gateways, Matchmaker, GameAllocator)
have no unit coverage of their own protocol/networking glue — that's
verified live, against the real Docker Compose stack, each time it
changes.

Build `Kung_Fu_Chess.exe` as **Win32/x86**, not x64 — the test binary's
`main()` (`test/doctest_main.cpp`) is only compiled into the Win32
configuration.

## Protocol

Live gameplay is a JSON envelope (`{"type": ..., "payload": ...}`) per
message over one WebSocket connection per client, routed by the WS
Gateway to whichever shard owns that connection's room. Everything
non-real-time (login, room creation/lookup, matchmaking, history) is
instead a REST call to the API Gateway:

| Method | Path | |
|---|---|---|
| POST | `/login` | username/password → token |
| POST | `/rooms` | create a room (Game Allocator picks the shard) |
| GET | `/rooms` | list known rooms |
| POST | `/rooms/{name}/join` | resolve an existing room's shard |
| POST | `/play` | matchmaking (blocks up to 60s for a match) |
| GET | `/history` | most recent finished games |
| GET | `/health` | liveness (also on WS Gateway/GameServer/Matchmaker/GameAllocator) |
| GET | `/metrics` | Prometheus text format (also on the other 4 services) |
