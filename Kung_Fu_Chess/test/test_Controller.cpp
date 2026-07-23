#include "../doctest.h"
#include "../client/input/Controller.h"
#include "../client/view/ViewConfig.h"
#include "../shared/protocol/MessageCodec.h"

namespace {
    GameSnapshot snapshotWith(std::vector<SnapshotPiece> pieces) {
        GameSnapshot snap;
        snap.board_width = 8;
        snap.board_height = 8;
        snap.pieces = std::move(pieces);
        return snap;
    }

    SnapshotPiece piece(Chess::Kind kind, Chess::Color color, Position pos) {
        SnapshotPiece p;
        p.kind = kind;
        p.color = color;
        p.cell = pos;
        p.state = Chess::State::Idle;
        return p;
    }

    // לחיצה על מרכז התא (row, col) בפיקסלים, לפי ViewConfig::CELL_SIZE
    void click(Controller& controller, int row, int col) {
        int x = col * ViewConfig::CELL_SIZE + 50;
        int y = row * ViewConfig::CELL_SIZE + 50;
        controller.handleMouseClick(x, y);
    }

    // Collects every message the controller tried to send, decoded, so
    // tests can assert on type/payload without any real network.
    struct RecordingSender {
        std::vector<Message> sent;
        Controller::Sender asSender() {
            return [this](const std::string& text) { sent.push_back(MessageCodec::decode(text)); };
        }
    };
}

TEST_CASE("click - קליק ראשון על כלי ואז קליק שני על יעד חוקי שולח הודעת MOVE") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3); // בחירת הצריח
    click(controller, 3, 6); // יעד חוקי וריק

    REQUIRE(sender.sent.size() == 1);
    CHECK(sender.sent[0].type == MessageType::Move);
    CHECK(sender.sent[0].payload.at("from").at("row") == 3);
    CHECK(sender.sent[0].payload.at("from").at("col") == 3);
    CHECK(sender.sent[0].payload.at("to").at("row") == 3);
    CHECK(sender.sent[0].payload.at("to").at("col") == 6);
}

TEST_CASE("click - קליק ראשון על תא ריק לא שולח כלום ולא בוחר") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 0, 0); // תא ריק
    CHECK(sender.sent.empty());
    CHECK_FALSE(controller.getSnapshot().has_selection);
}

TEST_CASE("click - קליק שני על אותו כלי נבחר שולח הודעת JUMP") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3);
    click(controller, 3, 3);

    REQUIRE(sender.sent.size() == 1);
    CHECK(sender.sent[0].type == MessageType::Jump);
    CHECK(sender.sent[0].payload.at("pos").at("row") == 3);
    CHECK(sender.sent[0].payload.at("pos").at("col") == 3);
    CHECK_FALSE(controller.getSnapshot().has_selection);
}

TEST_CASE("click - קליק מחוץ ללוח ללא בחירה קודמת לא שולח כלום") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    controller.handleMouseClick(-10, -10);
    CHECK(sender.sent.empty());
}

TEST_CASE("click - קליק מחוץ ללוח עם כלי נבחר מבטל בחירה ולא שולח מהלך") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3);
    controller.handleMouseClick(-10, -10);

    CHECK(sender.sent.empty());
    CHECK_FALSE(controller.getSnapshot().has_selection);
}

TEST_CASE("click - קליק שני על כלי ידידותי מחליף בחירה במקום לשלוח מהלך") {
    auto snap = snapshotWith({
        piece(Chess::Kind::Rook,   Chess::Color::White, {0, 0}),
        piece(Chess::Kind::Bishop, Chess::Color::White, {0, 2}),
    });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 0, 0);
    click(controller, 0, 2);

    CHECK(sender.sent.empty());
    CHECK(controller.getSnapshot().selected_cell == Position{0, 2});
}

TEST_CASE("click - פרש נבחר שקליק שני עליו כלי ידידותי שולח מהלך (כלל 8), לא מחליף בחירה") {
    auto snap = snapshotWith({
        piece(Chess::Kind::Knight, Chess::Color::White, {3, 3}),
        piece(Chess::Kind::Pawn,   Chess::Color::White, {1, 2}),
    });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3);
    click(controller, 1, 2);

    REQUIRE(sender.sent.size() == 1);
    CHECK(sender.sent[0].type == MessageType::Move);
}

TEST_CASE("click - קליק שני על כלי אויב שולח הודעת MOVE") {
    auto snap = snapshotWith({
        piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}),
        piece(Chess::Kind::Pawn, Chess::Color::Black, {3, 6}),
    });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3);
    click(controller, 3, 6);

    REQUIRE(sender.sent.size() == 1);
    CHECK(sender.sent[0].type == MessageType::Move);
}

TEST_CASE("getSnapshot - אין בחירה לפני קליק ראשון") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    CHECK_FALSE(controller.getSnapshot().has_selection);
}

TEST_CASE("getSnapshot - קליק ראשון על כלי חושף אותו כתא נבחר") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3);

    auto result = controller.getSnapshot();
    CHECK(result.has_selection);
    CHECK(result.selected_cell == Position{3, 3});
}

TEST_CASE("getSnapshot - אחרי שליחת הודעה הבחירה מתנקה מתמונת המצב") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    RecordingSender sender;
    BoardScale boardScale; // defaults to ViewConfig::CELL_SIZE, matching click()'s own math below
    Controller controller(snap, sender.asSender(), boardScale);

    click(controller, 3, 3);
    click(controller, 3, 6);

    CHECK_FALSE(controller.getSnapshot().has_selection);
}
