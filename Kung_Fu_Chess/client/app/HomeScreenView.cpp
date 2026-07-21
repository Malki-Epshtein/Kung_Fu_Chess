#include "HomeScreenView.h"
#include "HomeScreen.h"
#include "RoomDialog.h"
#include "../net/BlockingRequest.h"
#include "../../shared/protocol/Message.h"
#include "../../shared/protocol/MessageCodec.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>

namespace {
    constexpr const char* kHomeWindowName = "Kung Fu Chess - Home";

    // Set on the mouse callback, consumed by the loop in runHomeScreen()
    // below - deliberately NOT acted on inside the callback itself.
    // showRoomDialog() runs its own nested Win32 message loop, and calling
    // it from inside OpenCV's own mouse-event dispatch nests two message
    // loops on the same thread, which is fragile (observed: the dialog
    // closing itself immediately). Deferring the actual handling to a
    // clean point between cv::waitKey() calls avoids that entirely.
    HomeScreenChoice g_pendingChoice = HomeScreenChoice::None;

    // FindGame can take up to 60s to resolve (see WsServer.cpp) - blocking
    // the Home screen loop for that long would freeze the window (no
    // rendering, no ESC). So unlike Room's sendAndWaitForReply, Play
    // installs its own onMessage handler that just stashes any GAME_FOUND
    // push here; the main loop below polls it once per frame instead of
    // blocking on it.
    std::mutex                     g_findGameMutex;
    std::optional<nlohmann::json>  g_gameFoundResult;

    void drawButton(cv::Mat& canvas, const ButtonBounds& bounds, const std::string& label) {
        cv::Rect rect(bounds.x, bounds.y, bounds.width, bounds.height);
        cv::rectangle(canvas, rect, cv::Scalar(200, 200, 200), cv::FILLED);
        cv::rectangle(canvas, rect, cv::Scalar(80, 80, 80), 2);
        cv::putText(canvas, label, cv::Point(bounds.x + 20, bounds.y + bounds.height / 2 + 8),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
    }

    void onHomeMouseEvent(int event, int x, int y, int /*flags*/, void* /*userdata*/) {
        if (event != cv::EVENT_LBUTTONDOWN)
            return;
        HomeScreenChoice choice = HomeScreen::hitTest(x, y);
        if (choice != HomeScreenChoice::None)
            g_pendingChoice = choice;
    }

    // Sends CreateRoom/JoinRoom for `roomName` and blocks for the reply.
    // Empty roomName on failure (message logged); non-empty is the room
    // name to display, taken from the server's reply rather than echoing
    // back whatever was typed, since the two can differ in principle.
    struct RoomJoinOutcome {
        std::string    roomName;
        nlohmann::json moveLog;
        std::string    whiteName;
        int            whiteElo = 0;
        std::string    blackName;
        int            blackElo = 0;
    };

    RoomJoinOutcome sendRoomRequest(WsClient& client, RoomDialogResult::Action action, const std::string& roomName) {
        MessageType type = (action == RoomDialogResult::Action::Create) ? MessageType::CreateRoom : MessageType::JoinRoom;
        Message request{ type, { {"name", roomName} } };
        nlohmann::json reply = sendAndWaitForReply(client, request);

        if (!reply.value("success", false)) {
            std::string message = reply.value("message", "");
            std::cout << "[client] Room request failed: " << message << std::endl;
            showRoomError(message);
            return {};
        }
        return { reply.value("roomName", roomName), reply.value("moveLog", HomeScreenResult{}.moveLog),
                 reply.value("whiteName", std::string()), reply.value("whiteElo", 0),
                 reply.value("blackName", std::string()), reply.value("blackElo", 0) };
    }

    // Sends FindGame and returns immediately (does not wait for a result -
    // see g_gameFoundResult above). Installs its own onMessage handler
    // that only reacts to GAME_FOUND pushes; the immediate "searching for
    // opponent" ack (no "type" field) is deliberately ignored here.
    void startFindGame(WsClient& client) {
        {
            std::lock_guard<std::mutex> lock(g_findGameMutex);
            g_gameFoundResult.reset();
        }
        client.setOnMessage([](const std::string& text) {
            nlohmann::json j = nlohmann::json::parse(text);
            if (j.value("type", std::string()) != "GAME_FOUND")
                return;
            std::lock_guard<std::mutex> lock(g_findGameMutex);
            g_gameFoundResult = j;
        });
        client.send(MessageCodec::encode(Message{ MessageType::FindGame, {} }));
        std::cout << "[client] FindGame sent, searching for opponent" << std::endl;
    }

    // Non-blocking: returns the GAME_FOUND payload once startFindGame()'s
    // push has arrived, nullopt otherwise (keep waiting).
    std::optional<nlohmann::json> pollGameFound() {//אם כבר נמצא לי שחקן או לא
        std::lock_guard<std::mutex> lock(g_findGameMutex);
        if (!g_gameFoundResult)
            return std::nullopt;
        nlohmann::json payload = g_gameFoundResult->at("payload");
        g_gameFoundResult.reset();
        return payload;
    }
}

HomeScreenResult runHomeScreen(WsClient& client) {
    cv::namedWindow(kHomeWindowName);
    cv::setMouseCallback(kHomeWindowName, onHomeMouseEvent, nullptr);//כשמישהו ילחץ עם העכבר על הכפתור של הבית זה יפעיל את הפונקציה הזאת
    std::cout << "[client] Home screen opened" << std::endl;

    cv::Mat canvas(500, 800, CV_8UC3, cv::Scalar(40, 40, 40));
    drawButton(canvas, HomeScreen::playButtonBounds(), "Play");
    drawButton(canvas, HomeScreen::roomButtonBounds(), "Room");

    HomeScreenResult result;
    g_pendingChoice = HomeScreenChoice::None;
    bool searching  = false;
    while (true) {
        cv::Mat display = canvas.clone();
        if (searching)
            cv::putText(display, "Searching for opponent...", cv::Point(220, 420),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        cv::imshow(kHomeWindowName, display);

        int key = cv::waitKey(30);
        if (key == 27) { // ESC
            std::cout << "[client] Home screen closed: ESC pressed" << std::endl;
            break;
        }
        double visible = cv::getWindowProperty(kHomeWindowName, cv::WND_PROP_VISIBLE);
        if (visible < 1) {
            std::cout << "[client] Home screen closed: window no longer visible" << std::endl;
            break;
        }

        if (searching) {//אם אתה בחיפוש
            std::optional<nlohmann::json> found = pollGameFound();
            if (found) {
                searching = false;
                if (found->value("success", false)) {
                    result.joinedRoom = true;
                    result.roomName   = found->value("roomName", std::string());
                    result.moveLog    = found->value("moveLog", HomeScreenResult{}.moveLog);
                    result.whiteName  = found->value("whiteName", std::string());
                    result.whiteElo   = found->value("whiteElo", 0);
                    result.blackName  = found->value("blackName", std::string());
                    result.blackElo   = found->value("blackElo", 0);
                    break;
                }
                std::cout << "[client] FindGame failed: " << found->value("message", "") << std::endl;
                showRoomError(found->value("message", "no players available"));
            }
        } else if (g_pendingChoice == HomeScreenChoice::Play) {//אם לחצץ על PLAY
            std::cout << "[client] Home: Play clicked" << std::endl;
            g_pendingChoice = HomeScreenChoice::None;
            startFindGame(client);
            searching = true;
        } else if (g_pendingChoice == HomeScreenChoice::Room) {//אם לחצת על ROOM
            std::cout << "[client] Home: Room clicked" << std::endl;
            g_pendingChoice = HomeScreenChoice::None;

            RoomDialogResult dialogResult = showRoomDialog();//פותח את חלון הROOM ומחכה למשתמש לבחור אם הוא רוצה ליצור חדר או להצטרף לחדר קיים
            // Cancel, or Create/Join with nothing typed, is a no-op back
            // to the Home screen - matches the plan's Cancel/empty-field
            // behavior exactly.
            if (dialogResult.action == RoomDialogResult::Action::Cancel || dialogResult.roomName.empty())
                continue;

            RoomJoinOutcome outcome = sendRoomRequest(client, dialogResult.action, dialogResult.roomName);//שולח בקשה לפתיחת חדר או להצטרף לחדר קיים
            if (!outcome.roomName.empty()) {
                result.joinedRoom = true;
                result.roomName   = outcome.roomName;
                result.moveLog    = outcome.moveLog;
                result.whiteName  = outcome.whiteName;
                result.whiteElo   = outcome.whiteElo;
                result.blackName  = outcome.blackName;
                result.blackElo   = outcome.blackElo;
                break;
            }
            // Failure (e.g. "room already exists") already logged inside
            // sendRoomRequest() - stay on the Home screen so the user can
            // try again via the Room button.
        }
        g_pendingChoice = HomeScreenChoice::None; // stray clicks while searching are ignored
    }
    cv::destroyWindow(kHomeWindowName);
    return result;
}
