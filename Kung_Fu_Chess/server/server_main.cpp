#include "net/WsServer.h"
#include "app/session/SessionRegistry.h"
#include "db/IUserRepository.h"
#include "db/UserRepository.h"
#include "../shared/db/IRoomDirectory.h"
#include "../shared/db/IClientSessionStore.h"
#include "../shared/db/IGameHistoryRepository.h"
#ifdef KFC_HAVE_POSTGRES
#include "db/PostgresUserRepository.h"
#include "../shared/db/PostgresGameHistoryRepository.h"
#endif
#ifdef KFC_HAVE_REDIS
#include "../shared/db/RedisRoomDirectory.h"
#include "../shared/db/RedisClientSessionStore.h"
#endif
#include "../shared/bus/INatsClient.h"
#ifdef KFC_HAVE_NATS
#include "../shared/bus/NatsClient.h"
#endif
#include "../shared/bus/EventBus.h"
#include "../shared/log/Log.h"
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

namespace {
    constexpr uint16_t kPort = 9002;
    constexpr const char* kUserDbPath = "users.db";

    // DATABASE_URL selects Postgres (only possible on the Docker/Linux
    // build - see CMakeLists.txt's KFC_HAVE_POSTGRES). Unset, or running the
    // native Windows build (which never defines KFC_HAVE_POSTGRES at all,
    // so this whole branch compiles out), falls back to local SQLite -
    // identical behavior to before this backend existed.
    std::unique_ptr<IUserRepository> makeUserRepository() {
#ifdef KFC_HAVE_POSTGRES
        if (const char* url = std::getenv("DATABASE_URL")) {
            spdlog::info("connecting to Postgres");
            return std::make_unique<PostgresUserRepository>(url);
        }
#endif
        return std::make_unique<UserRepository>(kUserDbPath);
    }

    // Same DATABASE_URL gate as makeUserRepository() - unlike that one,
    // there's no SQLite/native-Windows equivalent, so this returns nullptr
    // rather than falling back to anything: GameHistoryService treats a
    // null repository as "don't record finished games" (see its header).
    std::unique_ptr<IGameHistoryRepository> makeGameHistoryRepository() {
#ifdef KFC_HAVE_POSTGRES
        if (const char* url = std::getenv("DATABASE_URL"))
            return std::make_unique<PostgresGameHistoryRepository>(url);
#endif
        return nullptr;
    }

    // REDIS_HOST selects a real room directory (Docker/Linux build only -
    // see CMakeLists.txt's KFC_HAVE_REDIS). Unset, or the native Windows
    // build, returns nullptr - SessionRegistry treats that as "no
    // directory", identical to before this existed.
    std::unique_ptr<IRoomDirectory> makeRoomDirectory() {
#ifdef KFC_HAVE_REDIS
        if (const char* host = std::getenv("REDIS_HOST")) {
            const char* portEnv = std::getenv("REDIS_PORT");
            int port = portEnv ? std::atoi(portEnv) : 6379;
            spdlog::info("connecting to Redis");
            return std::make_unique<RedisRoomDirectory>(host, port);
        }
#endif
        return nullptr;
    }

    // Same REDIS_HOST gate as the room directory above - shares the
    // connection info, just a different key prefix ("session:" vs "room:").
    // Lets EnterRoomHandler resolve a token minted by the API Gateway's
    // POST /login into an identity, regardless of which shard LOGIN
    // actually ran on. Null (native Windows, or no REDIS_HOST) disables
    // ENTER_ROOM entirely - see MessageDispatcher's constructor.
    std::unique_ptr<IClientSessionStore> makeSessionStore() {
#ifdef KFC_HAVE_REDIS
        if (const char* host = std::getenv("REDIS_HOST")) {
            const char* portEnv = std::getenv("REDIS_PORT");
            int port = portEnv ? std::atoi(portEnv) : 6379;
            return std::make_unique<RedisClientSessionStore>(host, port);
        }
#endif
        return nullptr;
    }

    // NATS_URL selects the event bus (Docker/Linux build only - see
    // CMakeLists.txt's KFC_HAVE_NATS). Unset, or the native Windows build,
    // returns nullptr - SessionRegistry treats that as "no bus", the same
    // no-op default every test already gets.
    std::unique_ptr<INatsClient> makeNatsClient() {
#ifdef KFC_HAVE_NATS
        if (const char* url = std::getenv("NATS_URL")) {
            spdlog::info("connecting to NATS");
            return std::make_unique<NatsClient>(url);
        }
#endif
        return nullptr;
    }

    // SHARD_ADDRESS env var wins if explicitly set (the Kubernetes path -
    // see k8s/gameserver-deployment.yaml's Downward API POD_IP -> SHARD_ADDRESS
    // substitution). Otherwise self-derive it from this container's own
    // hostname: Docker gives every container a unique hostname by default
    // (the container ID, unless overridden), writes a matching entry into
    // /etc/hosts for its own self-resolution, and its embedded DNS resolves
    // that same hostname for OTHER containers on the same user-defined
    // bridge network too - so "ws://<my hostname>:9002" is both unique and
    // reachable without any per-replica config, which is what makes
    // docker-compose.yml's gameserver service a single `replicas: N` block
    // instead of one hand-duplicated block per shard. GetComputerNameA is
    // the Windows equivalent, used only so Server.vcxproj keeps building -
    // matchmaking/allocation are already compiled out there regardless.
    std::string deriveShardAddress() {
        if (const char* explicitAddress = std::getenv("SHARD_ADDRESS"))
            return explicitAddress;

        char hostname[256] = {};
#ifdef _WIN32
        DWORD size = sizeof(hostname);
        if (!GetComputerNameA(hostname, &size))
            return "";
#else
        if (gethostname(hostname, sizeof(hostname)) != 0)
            return "";
#endif
        return std::string("ws://") + hostname + ":9002";
    }

    constexpr int kHeartbeatIntervalSeconds = 5;

    // Periodic "this shard is alive, here's its current load" publish -
    // the Game Allocator (Server_Design.md step 6) subscribes to this to
    // pick a least-loaded shard. Runs on its own thread rather than the
    // WsServer's io_context (which server.run() below owns and blocks on)
    // - NatsClient::publish is mutex-guarded and safe to call cross-thread,
    // same reasoning as RedisRoomDirectory's. No clean shutdown (matches
    // the rest of this process - see server.run()); the thread dies with
    // the process.
    void startHeartbeat(INatsClient& nats, SessionRegistry& registry, const std::string& shardAddress) {
        std::thread([&nats, &registry, shardAddress] {
            while (true) {
                nats.publish("shard.heartbeat",
                              { {"shard", shardAddress}, {"roomCount", registry.roomNames().size()} });
                std::this_thread::sleep_for(std::chrono::seconds(kHeartbeatIntervalSeconds));
            }
        }).detach();
    }
}

int main(int /*argc*/, char** /*argv*/) {
    Log::init("server");
    spdlog::info("starting on port {}", kPort);
    try {
        EventBus bus;
        std::unique_ptr<IRoomDirectory> directory = makeRoomDirectory();
        std::unique_ptr<INatsClient> nats = makeNatsClient();
        std::string shardAddressStr = deriveShardAddress();
        SessionRegistry registry(directory.get(), shardAddressStr, nats.get());
        std::unique_ptr<IUserRepository> users = makeUserRepository();
        std::unique_ptr<IClientSessionStore> sessionStore = makeSessionStore();
        std::unique_ptr<IGameHistoryRepository> gameHistory = makeGameHistoryRepository();
        if (nats)
            startHeartbeat(*nats, registry, shardAddressStr);
        WsServer server;
        server.run(kPort, registry, bus, *users, sessionStore.get(), nats.get(), shardAddressStr,
                   kDefaultTickMs, gameHistory.get());
    } catch (const std::exception& e) {
        spdlog::error("failed to start: {} (is port {} already in use by another instance?)", e.what(), kPort);
        return 1;
    }
    return 0;
}
