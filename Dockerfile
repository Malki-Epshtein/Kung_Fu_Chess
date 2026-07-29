# Builds only what CMakeLists.txt knows how to build (the headless C++
# server + the two gateway processes, plus the shared/ code they depend on)
# - the Windows GUI client stays on MSBuild/OpenCV and has no business in a
# Linux container. See CMakeLists.txt's own comment for why there are two
# build systems.

FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config libpq-dev libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

# cnats (NATS C client) has no bookworm apt package, unlike hiredis/libpq -
# vendored under ThirdParty/nats.c (same reasoning as sqlite3.c) instead of
# fetched at build time: this network TLS-intercepts outbound HTTPS (a
# content-filtering proxy), and a fresh container doesn't trust that
# proxy's root CA the way the host does, so `git clone`/curl to github.com
# fails from inside the build. Copied and built in its own layer, above the
# full `COPY . .` below, so editing our own source doesn't force a rebuild
# of this dependency every time. TLS/streaming/examples are disabled - our
# services only ever reach NATS over the compose-internal network, and
# don't use NATS Streaming. Installs to the default /usr/local prefix;
# CMakeLists.txt's find_path/find_library for NATS_INCLUDE_DIR/NATS_LIBRARY
# picks it up from there, and `ldconfig` at the end makes the runtime
# stages' dynamic linker resolve libnats.so without needing LD_LIBRARY_PATH.
COPY Kung_Fu_Chess/ThirdParty/nats.c /tmp/nats.c
RUN cmake -S /tmp/nats.c -B /tmp/nats.c/build -DCMAKE_BUILD_TYPE=Release \
        -DNATS_BUILD_STREAMING=OFF -DNATS_BUILD_EXAMPLES=OFF -DNATS_BUILD_WITH_TLS=OFF \
    && cmake --build /tmp/nats.c/build -j "$(nproc)" \
    && cmake --install /tmp/nats.c/build \
    && rm -rf /tmp/nats.c \
    && ldconfig

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target Server WsGateway ApiGateway Matchmaker GameAllocator -j "$(nproc)"

# --- runtime image: just the built binary, no compiler/toolchain ---
FROM debian:bookworm-slim AS server

# libpq/hiredis are shared libraries (unlike sqlite3.c, which is compiled
# directly into the binary) - need their runtime packages here even though
# the toolchain that built against -dev is gone.
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

# cnats has no apt package (see the builder stage) - copy the .so it built
# straight out of the builder's /usr/local/lib rather than rebuilding it
# here, then re-run ldconfig so the dynamic linker picks it up.
COPY --from=builder /usr/local/lib/libnats.so* /usr/local/lib/
RUN ldconfig

WORKDIR /data
COPY --from=builder /src/build/Server /usr/local/bin/Server

EXPOSE 9002
CMD ["/usr/local/bin/Server"]

# --- runtime image for the WS Gateway: no game/session/rules code in here
# at all, not even as an unused dependency - it was never linked into this
# binary in the first place. No Redis - a dumb byte-forwarding WebSocket
# proxy has nothing to look up there. It does need NATS now, though: shards
# are discovered dynamically from shard.heartbeat (see
# shared/discovery/ShardRegistry.h) instead of a static shard list, so
# scaling the game-server tier is a replica-count change, not a config
# edit. ---
FROM debian:bookworm-slim AS ws-gateway

COPY --from=builder /usr/local/lib/libnats.so* /usr/local/lib/
RUN ldconfig

COPY --from=builder /src/build/WsGateway /usr/local/bin/WsGateway

EXPOSE 8080
CMD ["/usr/local/bin/WsGateway"]

# --- runtime image for the API Gateway: same "no game/session/rules code"
# story as the WS Gateway above, but this one needs libhiredis (reads/
# writes the Redis-backed room directory and session store), libpq (reads
# game history for GET /history - see PostgresGameHistoryRepository), and
# NATS (same dynamic shard discovery as WsGateway - see ShardRegistry.h). ---
FROM debian:bookworm-slim AS api-gateway

RUN apt-get update && apt-get install -y --no-install-recommends \
    libhiredis0.14 \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libnats.so* /usr/local/lib/
RUN ldconfig

COPY --from=builder /src/build/ApiGateway /usr/local/bin/ApiGateway

EXPOSE 8081
CMD ["/usr/local/bin/ApiGateway"]

# --- runtime image for the Matchmaker service (Server_Design.md step 6):
# no game/session/rules code, but now needs libhiredis - the waiting pool
# is Redis-backed (RedisMatchPool) so multiple replicas can share it. ---
FROM debian:bookworm-slim AS matchmaker

RUN apt-get update && apt-get install -y --no-install-recommends \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libnats.so* /usr/local/lib/
RUN ldconfig

COPY --from=builder /src/build/Matchmaker /usr/local/bin/Matchmaker

CMD ["/usr/local/bin/Matchmaker"]

# --- runtime image for the Game Allocator service (Server_Design.md step
# 6): needs both NATS (matchmaking.matched/shard.heartbeat/shard.*.allocate)
# and Redis (RedisSequence, for globally-unique room names). ---
FROM debian:bookworm-slim AS game-allocator

RUN apt-get update && apt-get install -y --no-install-recommends \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libnats.so* /usr/local/lib/
RUN ldconfig

COPY --from=builder /src/build/GameAllocator /usr/local/bin/GameAllocator

CMD ["/usr/local/bin/GameAllocator"]
