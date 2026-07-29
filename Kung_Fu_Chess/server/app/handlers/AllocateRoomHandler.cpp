#include "AllocateRoomHandler.h"
#include "../session/SessionRegistry.h"
#include "../logic/StartingBoard.h"
#include <future>

AllocateRoomHandler::AllocateRoomHandler(SessionRegistry& registry, EventBus& bus,
                                          AttachBroadcaster attachBroadcaster, asio::io_context& ioContext)
    : registry_(registry), bus_(bus), attachBroadcaster_(std::move(attachBroadcaster)), ioContext_(ioContext) {}

nlohmann::json AllocateRoomHandler::handle(const nlohmann::json& request) {
    std::promise<nlohmann::json> promise;
    std::future<nlohmann::json> future = promise.get_future();
    ioContext_.post([this, request, &promise] {
        promise.set_value(doAllocate(request));
    });
    return future.get();
}

nlohmann::json AllocateRoomHandler::doAllocate(const nlohmann::json& request) {
    std::string roomName = request.at("roomName").get<std::string>();
    // Present (and non-empty) only for a PLAY match, where the Game
    // Allocator already knows both players up front - see
    // allocator_main.cpp's matchmaking.matched handler. A regular room's
    // allocation.request request has neither: the creator hasn't reserved
    // a seat, they'll simply arrive first via EnterRoom and get White by
    // the normal arrival-order rule (SessionRegistry::joinRoom).
    std::string whiteUsername = request.value("whiteUsername", std::string());
    std::string blackUsername = request.value("blackUsername", std::string());

    if (!registry_.createRoom(roomName, makeStartingBoard(), bus_, /*simultaneousMode=*/true))
        return { {"success", false}, {"message", "room already exists"} };

    attachBroadcaster_(roomName);
    if (!whiteUsername.empty() && !blackUsername.empty())
        registry_.reserveSeats(roomName, whiteUsername, blackUsername);

    return { {"success", true}, {"roomName", roomName} };
}
