#include "net/WsServer.h"
#include "app/session/SessionRegistry.h"
#include "db/IUserRepository.h"
#include "db/UserRepository.h"
#include "../shared/db/IRoomDirectory.h"
#include "../shared/db/IClientSessionStore.h"
#ifdef KFC_HAVE_POSTGRES
#include "db/PostgresUserRepository.h"
#endif
#ifdef KFC_HAVE_REDIS
#include "../shared/db/RedisRoomDirectory.h"
#include "../shared/db/RedisClientSessionStore.h"
#endif
#include "../shared/bus/EventBus.h"
#include "../shared/log/Log.h"
#include <cstdlib>
#include <memory>

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
}

int main(int /*argc*/, char** /*argv*/) {
    Log::init("server");
    spdlog::info("starting on port {}", kPort);
    try {
        EventBus bus;
        std::unique_ptr<IRoomDirectory> directory = makeRoomDirectory();
        const char* shardAddress = std::getenv("SHARD_ADDRESS");
        SessionRegistry registry(directory.get(), shardAddress ? shardAddress : "");
        std::unique_ptr<IUserRepository> users = makeUserRepository();
        std::unique_ptr<IClientSessionStore> sessionStore = makeSessionStore();
        WsServer server;
        server.run(kPort, registry, bus, *users, sessionStore.get());
    } catch (const std::exception& e) {
        spdlog::error("failed to start: {} (is port {} already in use by another instance?)", e.what(), kPort);
        return 1;
    }
    return 0;
}
