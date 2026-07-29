#include "GameStateMirrorService.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../../../shared/db/IGameStateStore.h"
#include "../../../shared/protocol/GameSnapshotCodec.h"

void GameStateMirrorService::attach(EventBus& bus, const std::string& roomName) {
    bus.subscribe(GameSession::moveLogTopic(roomName), [this, roomName](const nlohmann::json&) {
        mirror(roomName);
    });
    bus.subscribe(GameSession::gameEndedTopic(roomName), [this, roomName](const nlohmann::json&) {
        if (store_)
            store_->clear(roomName);
    });
}

void GameStateMirrorService::mirror(const std::string& roomName) {
    if (!store_)
        return; // no Redis configured - nothing to mirror to

    GameSession* session = registry_.room(roomName);
    if (!session)
        return;

    const RoomIdentity& identity = session->identity();
    store_->save(roomName,
                 GameSnapshotCodec::encode(session->engine().snapshot()).dump(),
                 session->fullMoveLog().dump(),
                 identity.whiteUsername, identity.whiteElo,
                 identity.blackUsername, identity.blackElo);
}
