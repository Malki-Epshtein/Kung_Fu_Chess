#include "HomeScreenView.h"
#include "HomeScreen.h"
#include "RoomDialog.h"
#include "../net/BlockingRequest.h"
#include "../../shared/protocol/Message.h"
#include "../../shared/protocol/MessageCodec.h"
#include "../../shared/log/Log.h"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace {
    constexpr const char* kHomeWindowName   = "Kung Fu Chess - Home";
    constexpr int         kHomeWindowWidth  = 800;
    constexpr int         kHomeWindowHeight = 500;

    // Reverse of the server's roleName() (server/app/session/RoleName.h) -
    // deliberately duplicated here rather than shared, same convention as
    // this codebase's other small per-file color<->string helpers (e.g.
    // GameSession.cpp's colorName), since sharing a header here would be the
    // only client/server cross-boundary include.
    Chess::Color colorFromRole(const std::string& role) {
        if (role == "White") return Chess::Color::White;
        if (role == "Black") return Chess::Color::Black;
        return Chess::Color::None;
    }

    // Set on the mouse callback, consumed by the loop in runHomeScreen()
    // below - deliberately NOT acted on inside the callback itself.
    // showRoomDialog() runs its own nested Win32 message loop, and calling
    // it from inside OpenCV's own mouse-event dispatch nests two message
    // loops on the same thread, which is fragile (observed: the dialog
    // closing itself immediately). Deferring the actual handling to a
    // clean point between cv::waitKey() calls avoids that entirely.
    HomeScreenChoice g_pendingChoice = HomeScreenChoice::None;

    // Updated on every mouse move (not just clicks) - read once per frame
    // in runHomeScreen()'s render loop below to drive the buttons' hover
    // highlight. Purely a rendering input; HomeScreen::hitTest() on click
    // is still the only thing that decides Play/Room/None.
    cv::Point g_mousePos(-1, -1);

    enum class HomeIcon { Play, Room };

    // BGR palette for the whole Home Screen - a dark charcoal base with a
    // warm bronze/gold accent, standing in for "dark metal / polished
    // wood" without needing any texture image assets.
    const cv::Scalar kBgCenter(34, 31, 28);
    const cv::Scalar kBgEdge(12, 11, 10);
    const cv::Scalar kAccent(58, 168, 214);
    const cv::Scalar kCream(232, 236, 238);
    const cv::Scalar kButtonFill(52, 47, 44);
    const cv::Scalar kButtonFillHover(70, 63, 58);
    const cv::Scalar kBevelLight(92, 84, 78);
    const cv::Scalar kBevelDark(18, 16, 14);

    // Radial vignette (lighter center, darker edges), drawn once into the
    // reusable background canvas below - a per-pixel loop is fine here
    // since it only runs once at Home Screen startup, never per frame.
    void drawVignette(cv::Mat& canvas) {
        cv::Point center(canvas.cols / 2, canvas.rows / 2);
        double maxDist = std::sqrt(double(center.x * center.x + center.y * center.y));
        for (int y = 0; y < canvas.rows; ++y) {
            for (int x = 0; x < canvas.cols; ++x) {
                double dx = x - center.x, dy = y - center.y;
                double t = std::min(1.0, std::sqrt(dx * dx + dy * dy) / maxDist);
                cv::Vec3b& px = canvas.at<cv::Vec3b>(y, x);
                for (int c = 0; c < 3; ++c)
                    px[c] = static_cast<uchar>(kBgCenter[c] * (1 - t) + kBgEdge[c] * t);
            }
        }
    }

    void drawTitle(cv::Mat& canvas) {
        const std::string title = "KUNG FU CHESS";
        int baseline = 0;
        double scale = 1.6;
        int thickness = 3;
        cv::Size textSize = cv::getTextSize(title, cv::FONT_HERSHEY_DUPLEX, scale, thickness, &baseline);
        int x = (canvas.cols - textSize.width) / 2;
        int y = 90;
        // Soft drop shadow behind the title, then a thin accent underline -
        // reads as "designed" using only OpenCV's built-in Hershey faces.
        cv::putText(canvas, title, cv::Point(x + 2, y + 2), cv::FONT_HERSHEY_DUPLEX, scale, cv::Scalar(0, 0, 0), thickness, cv::LINE_AA);
        cv::putText(canvas, title, cv::Point(x, y), cv::FONT_HERSHEY_DUPLEX, scale, kCream, thickness, cv::LINE_AA);
        cv::line(canvas, cv::Point(x, y + 18), cv::Point(x + textSize.width, y + 18), kAccent, 2, cv::LINE_AA);
    }

    void drawIcon(cv::Mat& canvas, const ButtonBounds& b, HomeIcon icon) {
        int cx = b.x + 34;
        int cy = b.y + b.height / 2;
        if (icon == HomeIcon::Play) {
            std::vector<cv::Point> tri = {
                cv::Point(cx - 8, cy - 12), cv::Point(cx - 8, cy + 12), cv::Point(cx + 12, cy)
            };
            cv::fillConvexPoly(canvas, tri, kCream, cv::LINE_AA);
        } else {
            // A simple door glyph: an outlined door with a small knob dot.
            cv::rectangle(canvas, cv::Rect(cx - 10, cy - 14, 20, 28), kCream, 2, cv::LINE_AA);
            cv::circle(canvas, cv::Point(cx + 4, cy), 2, kCream, cv::FILLED, cv::LINE_AA);
        }
    }

    // FindGame can take up to 60s to resolve (see WsServer.cpp) - blocking
    // the Home screen loop for that long would freeze the window (no
    // rendering, no ESC). So unlike Room's sendAndWaitForReply, Play
    // installs its own onMessage handler that just stashes any GAME_FOUND
    // push here; the main loop below polls it once per frame instead of
    // blocking on it.
    std::mutex                     g_findGameMutex;
    std::optional<nlohmann::json>  g_gameFoundResult;

    void drawButton(cv::Mat& canvas, const ButtonBounds& bounds, const std::string& label, HomeIcon icon, bool hovered) {
        cv::Rect rect(bounds.x, bounds.y, bounds.width, bounds.height);
        cv::rectangle(canvas, rect, hovered ? kButtonFillHover : kButtonFill, cv::FILLED);

        // Fake bevel: a lighter line along the top/left edge, darker along
        // bottom/right - cheap pseudo-3D relief with no texture image.
        cv::line(canvas, rect.tl(), cv::Point(rect.x + rect.width, rect.y), kBevelLight, 2, cv::LINE_AA);
        cv::line(canvas, rect.tl(), cv::Point(rect.x, rect.y + rect.height), kBevelLight, 2, cv::LINE_AA);
        cv::line(canvas, cv::Point(rect.x, rect.y + rect.height), cv::Point(rect.x + rect.width, rect.y + rect.height), kBevelDark, 2, cv::LINE_AA);
        cv::line(canvas, cv::Point(rect.x + rect.width, rect.y), cv::Point(rect.x + rect.width, rect.y + rect.height), kBevelDark, 2, cv::LINE_AA);

        // Accent trim, thicker/brighter-reading when hovered - stands in
        // for the prompt's "subtle glow" without an actual blur pass.
        cv::rectangle(canvas, rect, kAccent, hovered ? 2 : 1, cv::LINE_AA);

        drawIcon(canvas, bounds, icon);
        cv::putText(canvas, label, cv::Point(bounds.x + 62, bounds.y + bounds.height / 2 + 8),
                    cv::FONT_HERSHEY_DUPLEX, 0.8, kCream, 1, cv::LINE_AA);
    }

    void onHomeMouseEvent(int event, int x, int y, int /*flags*/, void* /*userdata*/) {
        g_mousePos = cv::Point(x, y);
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
        Chess::Color   role = Chess::Color::None;
    };

    // Sends AUTH{token} over `client` and blocks for the reply - establishes
    // this connection's identity (ClientSessionRegistry), which FindGame
    // needs and EnterRoom re-derives from the token anyway (harmless -
    // ClientSessionRegistry::onLogin just overwrites the same entry).
    // Shared by both the Play and Room paths below, since every fresh
    // connection this client opens needs it exactly once, right after open.
    bool authenticateConnection(WsClient& client, const std::string& token) {
        Message request{ MessageType::Auth, { {"token", token} } };
        nlohmann::json reply = sendAndWaitForReply(client, request, MessageType::Auth);
        if (!reply.value("success", false)) {
            std::string message = reply.value("message", "");
            spdlog::info("Auth failed: {}", message);
            showRoomError(message);
            return false;
        }
        return true;
    }

    // Resolves `roomName` over REST (POST /rooms for Create, POST
    // /rooms/{name}/join for Join) to find which shard hosts it, connects
    // there (X-Shard-Hint - see ConnectFn), authenticates, then claims the
    // seat with EnterRoom. Replaces the old direct CREATE_ROOM/JOIN_ROOM
    // WebSocket exchange now that LOGIN (and so this connection's identity)
    // no longer happens on the WebSocket at all.
    RoomJoinOutcome sendRoomRequest(HttpClient& api, const std::string& token, const ConnectFn& connect,
                                     RoomDialogResult::Action action, const std::string& roomName) {
        bool isCreate = action == RoomDialogResult::Action::Create;
        std::string path = isCreate ? "/rooms" : ("/rooms/" + roomName + "/join");
        nlohmann::json body = isCreate ? nlohmann::json{ {"token", token}, {"name", roomName} }
                                        : nlohmann::json{ {"token", token} };
        nlohmann::json restReply = api.post(path, body);

        if (!restReply.value("success", false)) {
            std::string message = restReply.value("message", "");
            spdlog::info("Room request failed: {}", message);
            showRoomError(message);
            return {};
        }
        std::string shard          = restReply.value("shard", std::string());
        std::string resolvedName   = restReply.value("roomName", roomName);

        WsClient& client = connect(shard);
        if (!authenticateConnection(client, token))
            return {};

        Message enterRequest{ MessageType::EnterRoom, { {"token", token}, {"roomName", resolvedName} } };
        nlohmann::json reply = sendAndWaitForReply(client, enterRequest, MessageType::EnterRoom);

        if (!reply.value("success", false)) {
            std::string message = reply.value("message", "");
            spdlog::info("EnterRoom failed: {}", message);
            showRoomError(message);
            return {};
        }
        return { reply.value("roomName", resolvedName), reply.value("moveLog", HomeScreenResult{}.moveLog),
                 reply.value("whiteName", std::string()), reply.value("whiteElo", 0),
                 reply.value("blackName", std::string()), reply.value("blackElo", 0),
                 colorFromRole(reply.value("role", std::string())) };
    }

    // Sends FindGame and returns immediately (does not wait for a result -
    // see g_gameFoundResult above). Installs its own onMessage handler
    // that only reacts to GAME_FOUND pushes; the immediate "searching for
    // opponent" ack (tagged "type":"FIND_GAME", not "GAME_FOUND") is
    // deliberately ignored here.
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
        spdlog::info("FindGame sent, searching for opponent");
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

HomeScreenResult runHomeScreen(HttpClient& api, const std::string& token, ConnectFn connect) {
    cv::namedWindow(kHomeWindowName);
    cv::setMouseCallback(kHomeWindowName, onHomeMouseEvent, nullptr);//כשמישהו ילחץ עם העכבר על הכפתור של הבית זה יפעיל את הפונקציה הזאת
    spdlog::info("Home screen opened");

    // Background + title are static (drawn once); buttons are redrawn every
    // frame below since their hover highlight depends on the live mouse
    // position - hitTest()/bounds themselves are unchanged either way.
    cv::Mat background(kHomeWindowHeight, kHomeWindowWidth, CV_8UC3);
    drawVignette(background);
    drawTitle(background);

    HomeScreenResult result;
    g_pendingChoice = HomeScreenChoice::None;
    g_mousePos = cv::Point(-1, -1);
    bool searching  = false;
    while (true) {
        cv::Mat display = background.clone();
        HomeScreenChoice hovered = HomeScreen::hitTest(g_mousePos.x, g_mousePos.y);
        drawButton(display, HomeScreen::playButtonBounds(), "PLAY", HomeIcon::Play, hovered == HomeScreenChoice::Play);
        drawButton(display, HomeScreen::roomButtonBounds(), "ROOM", HomeIcon::Room, hovered == HomeScreenChoice::Room);
        if (searching)
            cv::putText(display, "Searching for opponent...", cv::Point(220, 420),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, kAccent, 2, cv::LINE_AA);
        cv::imshow(kHomeWindowName, display);

        int key = cv::waitKey(30);
        if (key == 27) { // ESC
            spdlog::info("Home screen closed: ESC pressed");
            break;
        }
        double visible = cv::getWindowProperty(kHomeWindowName, cv::WND_PROP_VISIBLE);
        if (visible < 1) {
            spdlog::info("Home screen closed: window no longer visible");
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
                    result.role       = colorFromRole(found->value("role", std::string()));
                    break;
                }
                spdlog::info("FindGame failed: {}", found->value("message", ""));
                showRoomError(found->value("message", "no players available"));
            }
        } else if (g_pendingChoice == HomeScreenChoice::Play) {//אם לחצץ על PLAY
            spdlog::info("Home: Play clicked");
            g_pendingChoice = HomeScreenChoice::None;
            // No room known yet - round-robin (empty shard hint), same as
            // before the Gateway existed.
            WsClient& client = connect("");
            if (!authenticateConnection(client, token))
                continue;
            startFindGame(client);
            searching = true;
        } else if (g_pendingChoice == HomeScreenChoice::Room) {//אם לחצת על ROOM
            spdlog::info("Home: Room clicked");
            g_pendingChoice = HomeScreenChoice::None;

            RoomDialogResult dialogResult = showRoomDialog();//פותח את חלון הROOM ומחכה למשתמש לבחור אם הוא רוצה ליצור חדר או להצטרף לחדר קיים
            // Cancel, or Create/Join with nothing typed, is a no-op back
            // to the Home screen - matches the plan's Cancel/empty-field
            // behavior exactly.
            if (dialogResult.action == RoomDialogResult::Action::Cancel || dialogResult.roomName.empty())
                continue;

            RoomJoinOutcome outcome = sendRoomRequest(api, token, connect, dialogResult.action, dialogResult.roomName);//שולח בקשה לפתיחת חדר או להצטרף לחדר קיים
            if (!outcome.roomName.empty()) {
                result.joinedRoom = true;
                result.roomName   = outcome.roomName;
                result.moveLog    = outcome.moveLog;
                result.whiteName  = outcome.whiteName;
                result.whiteElo   = outcome.whiteElo;
                result.blackName  = outcome.blackName;
                result.blackElo   = outcome.blackElo;
                result.role       = outcome.role;
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
