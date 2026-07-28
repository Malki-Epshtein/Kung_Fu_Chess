# Builds only what CMakeLists.txt knows how to build (the headless C++
# server + the two gateway processes, plus the shared/ code they depend on)
# - the Windows GUI client stays on MSBuild/OpenCV and has no business in a
# Linux container. See CMakeLists.txt's own comment for why there are two
# build systems.

FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config libpq-dev libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target Server WsGateway ApiGateway -j "$(nproc)"

# --- runtime image: just the built binary, no compiler/toolchain ---
FROM debian:bookworm-slim AS server

# libpq/hiredis are shared libraries (unlike sqlite3.c, which is compiled
# directly into the binary) - need their runtime packages here even though
# the toolchain that built against -dev is gone.
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /data
COPY --from=builder /src/build/Server /usr/local/bin/Server

EXPOSE 9002
CMD ["/usr/local/bin/Server"]

# --- runtime image for the WS Gateway: no game/session/rules code in here
# at all, not even as an unused dependency - it was never linked into this
# binary in the first place. No Redis either (see CMakeLists.txt's own
# comment on why WsGateway stays dependency-free) - a dumb byte-forwarding
# WebSocket proxy has nothing to look up. ---
FROM debian:bookworm-slim AS ws-gateway

COPY --from=builder /src/build/WsGateway /usr/local/bin/WsGateway

EXPOSE 8080
CMD ["/usr/local/bin/WsGateway"]

# --- runtime image for the API Gateway: same "no game/session/rules code"
# story as the WS Gateway above, but this one does need libhiredis at
# runtime - ApiGateway reads/writes the Redis-backed room directory and
# session store (see CMakeLists.txt's own comment). ---
FROM debian:bookworm-slim AS api-gateway

RUN apt-get update && apt-get install -y --no-install-recommends \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/ApiGateway /usr/local/bin/ApiGateway

EXPOSE 8081
CMD ["/usr/local/bin/ApiGateway"]
