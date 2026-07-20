#include "../doctest.h"
#include "../shared/input/ClickResolver.h"

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
}

TEST_CASE("ClickResolver - קליק ראשון על כלי בוחר אותו") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    auto outcome = ClickResolver::resolve(snap, std::nullopt, {3, 3});
    CHECK(outcome.type == ClickOutcomeType::Selected);
    CHECK(outcome.pos == Position{3, 3});
}

TEST_CASE("ClickResolver - קליק ראשון על תא ריק לא עושה כלום") {
    auto snap = snapshotWith({});
    auto outcome = ClickResolver::resolve(snap, std::nullopt, {3, 3});
    CHECK(outcome.type == ClickOutcomeType::NoOp);
}

TEST_CASE("ClickResolver - קליק שני על אותו תא נבחר שולח קפיצה") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    auto outcome = ClickResolver::resolve(snap, Position{3, 3}, {3, 3});
    CHECK(outcome.type == ClickOutcomeType::SendJump);
    CHECK(outcome.pos == Position{3, 3});
}

TEST_CASE("ClickResolver - קליק שני על תא ריק שולח מהלך") {
    auto snap = snapshotWith({ piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}) });
    auto outcome = ClickResolver::resolve(snap, Position{3, 3}, {3, 6});
    CHECK(outcome.type == ClickOutcomeType::SendMove);
    CHECK(outcome.from == Position{3, 3});
    CHECK(outcome.to == Position{3, 6});
}

TEST_CASE("ClickResolver - קליק שני על כלי אויב שולח מהלך אכילה") {
    auto snap = snapshotWith({
        piece(Chess::Kind::Rook, Chess::Color::White, {3, 3}),
        piece(Chess::Kind::Pawn, Chess::Color::Black, {3, 6}),
    });
    auto outcome = ClickResolver::resolve(snap, Position{3, 3}, {3, 6});
    CHECK(outcome.type == ClickOutcomeType::SendMove);
}

TEST_CASE("ClickResolver - קליק שני על כלי ידידותי מחליף בחירה") {
    auto snap = snapshotWith({
        piece(Chess::Kind::Rook,   Chess::Color::White, {0, 0}),
        piece(Chess::Kind::Bishop, Chess::Color::White, {0, 2}),
    });
    auto outcome = ClickResolver::resolve(snap, Position{0, 0}, {0, 2});
    CHECK(outcome.type == ClickOutcomeType::Selected);
    CHECK(outcome.pos == Position{0, 2});
}

TEST_CASE("ClickResolver - פרש נבחר שקליק שני עליו כלי ידידותי שולח מהלך (לא מחליף בחירה)") {
    auto snap = snapshotWith({
        piece(Chess::Kind::Knight, Chess::Color::White, {3, 3}),
        piece(Chess::Kind::Pawn,   Chess::Color::White, {1, 2}),
    });
    auto outcome = ClickResolver::resolve(snap, Position{3, 3}, {1, 2});
    CHECK(outcome.type == ClickOutcomeType::SendMove);
    CHECK(outcome.from == Position{3, 3});
    CHECK(outcome.to == Position{1, 2});
}
