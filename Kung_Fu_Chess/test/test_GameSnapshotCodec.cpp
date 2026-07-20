#include "../doctest.h"
#include "../shared/protocol/GameSnapshotCodec.h"

TEST_CASE("GameSnapshotCodec - round-trip משמר את שדות הלוח והכלי") {
    GameSnapshot original;
    original.board_width  = 8;
    original.board_height = 8;
    original.game_over    = false;
    original.has_selection = true;
    original.selected_cell = { 2, 3 };
    original.legalMoves    = { {1, 1}, {2, 2} };

    SnapshotPiece piece;
    piece.kind = Chess::Kind::Queen;
    piece.color = Chess::Color::White;
    piece.cell = { 4, 4 };
    piece.state = Chess::State::Moving;
    piece.elapsed_in_state_ms = 123;
    piece.targetCell = { 4, 5 };
    piece.travelProgress = 0.5;
    piece.restProgress = 0.0;
    original.pieces.push_back(piece);

    GameSnapshot decoded = GameSnapshotCodec::decode(GameSnapshotCodec::encode(original));

    CHECK(decoded.board_width == original.board_width);
    CHECK(decoded.board_height == original.board_height);
    CHECK(decoded.game_over == original.game_over);
    CHECK(decoded.has_selection == original.has_selection);
    CHECK(decoded.selected_cell == original.selected_cell);
    CHECK(decoded.legalMoves == original.legalMoves);

    REQUIRE(decoded.pieces.size() == 1);
    CHECK(decoded.pieces[0].kind == Chess::Kind::Queen);
    CHECK(decoded.pieces[0].color == Chess::Color::White);
    CHECK(decoded.pieces[0].cell == Position{4, 4});
    CHECK(decoded.pieces[0].state == Chess::State::Moving);
    CHECK(decoded.pieces[0].elapsed_in_state_ms == 123);
    CHECK(decoded.pieces[0].targetCell == Position{4, 5});
    CHECK(decoded.pieces[0].travelProgress == doctest::Approx(0.5));
}

TEST_CASE("GameSnapshotCodec - לוח ריק בלי כלים עובר round-trip") {
    GameSnapshot original;
    original.board_width  = 8;
    original.board_height = 8;
    original.game_over    = true;

    GameSnapshot decoded = GameSnapshotCodec::decode(GameSnapshotCodec::encode(original));

    CHECK(decoded.pieces.empty());
    CHECK(decoded.game_over);
}
