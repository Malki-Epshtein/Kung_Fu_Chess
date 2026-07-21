#include "GraphicalApplication.h"
#include "HomeScreenView.h"
#include "../view/ViewConfig.h"
#include "../../shared/protocol/GameSnapshotCodec.h"
#include "../../shared/protocol/CaptureEventCodec.h"
#include <opencv2/opencv.hpp>
#include <iostream>

namespace {
    // "White"/"Black" -> the other color's display name, for the
    // disconnect-countdown message (Stage D).
    const char* opponentName(const std::string& color) {
        return color == "White" ? "Black" : "White";
    }

    // "name (elo)", matching how real chess sites always pair a player's
    // name with their rating rather than showing either alone. An empty
    // name means that seat isn't filled yet (e.g. the room's creator,
    // still waiting for Black to join).
    std::string playerLabel(const std::string& name, int elo) {
        if (name.empty())
            return "Waiting for player...";
        return name + " (" + std::to_string(elo) + ")";
    }

    void onMouseEvent(int event, int x, int y, int /*flags*/, void* userdata) {
        if (event != cv::EVENT_LBUTTONDOWN)
            return;
        static_cast<GraphicalApplication*>(userdata)->onMouseClick(x, y);
    }
}

void GraphicalApplication::onMouseClick(int x, int y) {
    // The board is now drawn centered, offset right by the left panel's
    // width and down by the top frame margin (see ViewConfig::PANEL_WIDTH
    // / BOARD_MARGIN) - correct raw window coordinates back to board-local
    // ones here, at the OS input boundary, so BoardMapper/Controller stay
    // unaware of any rendering layout. A click landing inside a side panel
    // or the frame margin becomes a negative coordinate, which Controller's
    // existing bounds check already treats as out-of-board.
    controller.handleMouseClick(x - ViewConfig::PANEL_WIDTH, y - ViewConfig::BOARD_MARGIN);
    soundPlayer.play("click.wav");
}

void GraphicalApplication::onMessage(const std::string& text) {
    // Runs on WsClient's network thread. The server sends three
    // differently-shaped JSON payloads on the same connection: a broadcast
    // GameSnapshot (un-enveloped, has "board_width"), a direct
    // command-reply ack (un-enveloped, has "success"), and enveloped
    // server->client pushes (has "type") like MOVE_LOGGED/CAPTURE_EVENT.
    // Only the first two are handled elsewhere/below; the enveloped pushes
    // are handled first here since they'd otherwise be silently dropped by
    // the "board_width" check below (they have neither key).
    try {
        nlohmann::json j = nlohmann::json::parse(text);

        if (j.value("type", std::string()) == "MOVE_LOGGED") {
            MoveLogEvent event = MoveLogCodec::decode(j.at("payload"));
            std::lock_guard<std::mutex> lock(snapshotMutex);
            if (event.color == Chess::Color::White)
                networkWhiteMoves.push_back(event.entry);
            else if (event.color == Chess::Color::Black)
                networkBlackMoves.push_back(event.entry);
            soundPlayer.play("move.wav");
            return;
        }

        // A real capture on the server reaches the client as a
        // CAPTURE_EVENT push - play the capture sound.
        if (j.value("type", std::string()) == "CAPTURE_EVENT") {
            CaptureEvent event = CaptureEventCodec::decode(j.at("payload"));
            std::cout << "[client] capture event received: kind=" << static_cast<int>(event.kind)
                       << " color=" << static_cast<int>(event.color)
                       << " cell=" << event.cell << std::endl;
            soundPlayer.play("capture.wav");
            return;
        }

        if (!j.contains("board_width"))
            return;

        GameSnapshot decoded = GameSnapshotCodec::decode(j);

        // Stage D: the "disconnect" key rides along on the same broadcast
        // rather than being a separate message shape - present only while a
        // seated player's grace-period countdown is active.
        bool active = false;
        std::string message;
        if (j.contains("disconnect") && j["disconnect"].value("active", false)) {
            std::string color = j["disconnect"].value("color", "");
            int secondsRemaining = j["disconnect"].value("secondsRemaining", 0);
            active = true;
            message = secondsRemaining > 0
                ? color + " disconnected - auto-resign in " + std::to_string(secondsRemaining) + "s"
                : color + " resigned (disconnected) - " + opponentName(color) + " wins!";
        }

        // Stage I: score rides along the same way disconnect does.
        int whiteScore = j.value("score", nlohmann::json::object()).value("white", 0);
        int blackScore = j.value("score", nlohmann::json::object()).value("black", 0);

        // Step 4: identity (name/elo/spectatorCount) rides the same way -
        // refreshed every tick, not just once at room-join time.
        nlohmann::json identity = j.value("identity", nlohmann::json::object());
        std::string whiteName    = identity.value("whiteName", std::string());
        int         whiteElo     = identity.value("whiteElo", 0);
        std::string blackName    = identity.value("blackName", std::string());
        int         blackElo     = identity.value("blackElo", 0);
        int         spectatorCount = identity.value("spectatorCount", 0);

        std::lock_guard<std::mutex> lock(snapshotMutex);
        networkSnapshot = std::move(decoded);
        networkDisconnectActive = active;
        networkDisconnectMessage = std::move(message);
        networkWhiteScore = whiteScore;
        networkBlackScore = blackScore;
        networkWhiteName = std::move(whiteName);
        networkWhiteElo = whiteElo;
        networkBlackName = std::move(blackName);
        networkBlackElo = blackElo;
        networkSpectatorCount = spectatorCount;
    } catch (const std::exception& e) {
        std::cerr << "[client] failed to parse server message: " << e.what() << std::endl;
    }
}

void GraphicalApplication::run() {
    // Shown first; blocks until a room is actually created/joined (Play
    // isn't wired to anything yet - Stage H) or the user closes the
    // window, which quits the app the same way closing the game window
    // does today - there's nothing useful to show without a room.
    HomeScreenResult home = runHomeScreen(client);//כל מסך הבית קורה עעכשיו
    if (!home.joinedRoom) {
        std::cout << "[client] exiting: Home screen closed without joining a room" << std::endl;
        return;
    }
    view.setRoomName(home.roomName);//שומרים את שם החדר שהמשתמש הצטרף אליו כדי להציג אותו על המסך
    view.setPlayerNames(playerLabel(home.whiteName, home.whiteElo), playerLabel(home.blackName, home.blackElo));

    // One-time backfill for a room already in progress (Stage I) - safe to
    // touch networkWhiteMoves/networkBlackMoves without the lock here,
    // since onMessage isn't installed yet (a couple of lines below).
    MoveLogBundle backfill = MoveLogCodec::decodeAll(home.moveLog);
    networkWhiteMoves = std::move(backfill.white);
    networkBlackMoves = std::move(backfill.black);

    // Only installed now that a room is actually joined - runHomeScreen()
    // needed the connection free for its own blocking CreateRoom/JoinRoom
    // exchange (same reasoning as LoginFlow, in the constructor).
    client.setOnMessage([this](const std::string& text) { onMessage(text); });//אני מחכה עש שיהיה לי תשובה

    cv::namedWindow(ViewConfig::WINDOW_NAME);
    cv::setMouseCallback(ViewConfig::WINDOW_NAME, onMouseEvent, this);
    std::cout << "[client] window opened, entering render loop" << std::endl;

    int frame = 0;
    bool wasGameOver = false;
    while (true) {
        bool        disconnectActive;
        std::string disconnectMessage;
        int         whiteScore, blackScore;
        std::vector<MoveEntry> whiteMoves, blackMoves;
        std::string whiteName, blackName;
        int         whiteElo, blackElo, spectatorCount;
        {
            std::lock_guard<std::mutex> lock(snapshotMutex);
            latestSnapshot = networkSnapshot;
            disconnectActive = networkDisconnectActive;
            disconnectMessage = networkDisconnectMessage;
            whiteScore = networkWhiteScore;
            blackScore = networkBlackScore;
            whiteMoves = networkWhiteMoves;
            blackMoves = networkBlackMoves;
            whiteName = networkWhiteName;
            whiteElo = networkWhiteElo;
            blackName = networkBlackName;
            blackElo = networkBlackElo;
            spectatorCount = networkSpectatorCount;
        }
        // Edge-triggered (only the frame game_over first becomes true, not
        // every frame it stays true) - game_over already rides on every
        // snapshot (see ImageView's own game-over message), so no new
        // server-side event is needed just for this sound.
        if (latestSnapshot.game_over && !wasGameOver)
            soundPlayer.play("game over.wav");
        wasGameOver = latestSnapshot.game_over;

        view.setDisconnectStatus(disconnectActive, disconnectMessage);
        view.setScore(whiteScore, blackScore);
        view.setMoveLog(std::move(whiteMoves), std::move(blackMoves));
        view.setPlayerNames(playerLabel(whiteName, whiteElo), playerLabel(blackName, blackElo));
        view.setSpectatorCount(spectatorCount);
        try {
            view.render(controller.getSnapshot());
        } catch (const std::exception& e) {
            // A render failure used to crash the whole client with zero
            // diagnostic output (the window would just vanish) - surfacing
            // it here at least tells us what actually went wrong.
            std::cerr << "[client] render() failed at frame " << frame << ": " << e.what() << std::endl;
            break;
        }
        ++frame;

        int key = cv::waitKey(30);
        if (key == 27) { // ESC
            std::cout << "[client] exiting: ESC pressed at frame " << frame << std::endl;
            break;
        }
        double visible = cv::getWindowProperty(ViewConfig::WINDOW_NAME, cv::WND_PROP_VISIBLE);
        if (visible < 1) {
            std::cout << "[client] exiting: window no longer visible (property=" << visible
                       << ") at frame " << frame << std::endl;
            break;
        }
    }
}
