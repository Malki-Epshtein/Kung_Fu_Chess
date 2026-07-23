#include "GraphicalApplication.h"
#include "HomeScreenView.h"
#include "../view/ViewConfig.h"
#include <opencv2/opencv.hpp>
#include <iostream>

namespace {
    // "name (elo)", matching how real chess sites always pair a player's
    // name with their rating rather than showing either alone. An empty
    // name means that seat isn't filled yet (e.g. the room's creator,
    // still waiting for Black to join).
    std::string playerLabel(const std::string& name, int elo) {
        if (name.empty())
            return "Waiting for player...";
        return name + " (" + std::to_string(elo) + ")";
    }

    // Forwards every event OpenCV's HighGUI reports - down/move/up are all
    // needed now for the drag-to-resize handle (Phase 3), not just clicks.
    void mouseCallback(int event, int x, int y, int /*flags*/, void* userdata) {
        static_cast<GraphicalApplication*>(userdata)->onMouseEvent(event, x, y);
    }
}

void GraphicalApplication::onMouseEvent(int event, int x, int y) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (boardScale.isInHandle(latestSnapshot.board_width, latestSnapshot.board_height, x, y)) {
            // Grabbing the handle starts a drag instead of a board click -
            // deliberately returns here so this same press is never also
            // read as a piece selection.
            resizing_          = true;
            dragStartX_        = x;
            dragStartY_        = y;
            dragStartCellSize_ = boardScale.cellSize();
            return;
        }
        onMouseClick(x, y);
        return;
    }

    if (event == cv::EVENT_MOUSEMOVE) {
        if (!resizing_ || latestSnapshot.board_width <= 0)
            return;
        // Diagonal drag distance from the grab point - growing/shrinking
        // both axes together, matching a corner handle's usual feel.
        int delta = ((x - dragStartX_) + (y - dragStartY_)) / 2;
        boardScale.setCellSize(dragStartCellSize_ + delta / latestSnapshot.board_width);
        return;
    }

    if (event == cv::EVENT_LBUTTONUP)
        resizing_ = false;
}

void GraphicalApplication::onMouseClick(int x, int y) {
    // The board is now drawn centered, offset right by the left panel's
    // width and down by the top frame margin (see ViewConfig::PANEL_WIDTH
    // / BOARD_MARGIN) - correct raw window coordinates back to board-local
    // ones here, at the OS input boundary, so BoardMapper/Controller stay
    // unaware of any rendering layout. A click landing inside a side panel
    // or the frame margin becomes a negative coordinate, which Controller's
    // existing bounds check already treats as out-of-board.
    ClickOutcomeType outcome = controller.handleMouseClick(x - ViewConfig::PANEL_WIDTH, y - ViewConfig::BOARD_MARGIN);
    soundPlayer.play(outcome == ClickOutcomeType::SendJump ? "jump.wav" : "click.wav");
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
    // Restricts selection to this player's own pieces from here on (see
    // Controller::setMyColor/ClickResolver's comments) - unknown any
    // earlier, since role only comes back on the room join/GameFound reply.
    controller.setMyColor(home.role);

    // One-time backfill for a room already in progress (Stage I) - safe to
    // call before networkHandler.onMessage is ever installed (a couple of
    // lines below), so there's no concurrent access yet.
    MoveLogBundle backfill = MoveLogCodec::decodeAll(home.moveLog);
    networkHandler.seedMoveLog(std::move(backfill.white), std::move(backfill.black));

    // Only installed now that a room is actually joined - runHomeScreen()
    // needed the connection free for its own blocking CreateRoom/JoinRoom
    // exchange (same reasoning as LoginFlow, in the constructor).
    client.setOnMessage([this](const std::string& text) { networkHandler.onMessage(text); });//אני מחכה עש שיהיה לי תשובה

    cv::namedWindow(ViewConfig::WINDOW_NAME);
    cv::setMouseCallback(ViewConfig::WINDOW_NAME, mouseCallback, this);
    std::cout << "[client] window opened, entering render loop" << std::endl;

    int frame = 0;
    bool wasGameOver = false;
    while (true) {
        NetworkState net = networkHandler.currentState();
        // Controller holds a reference to latestSnapshot specifically (bound
        // once, at construction) - assign into it in place rather than
        // rebinding to net.snapshot directly.
        latestSnapshot = std::move(net.snapshot);

        // Edge-triggered (only the frame game_over first becomes true, not
        // every frame it stays true) - game_over already rides on every
        // snapshot (see ImageView's own game-over message), so no new
        // server-side event is needed just for this sound.
        if (latestSnapshot.game_over && !wasGameOver)
            soundPlayer.play("game over.wav");
        wasGameOver = latestSnapshot.game_over;

        view.setDisconnectStatus(net.disconnectActive, net.disconnectMessage);
        view.setScore(net.whiteScore, net.blackScore);
        view.setMoveLog(std::move(net.whiteMoves), std::move(net.blackMoves));
        view.setPlayerNames(playerLabel(net.whiteName, net.whiteElo), playerLabel(net.blackName, net.blackElo));
        view.setSpectatorCount(net.spectatorCount);
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
