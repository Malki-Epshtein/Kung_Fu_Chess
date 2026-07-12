#include "../doctest.h"
#include "../view/ImageView.h"
#include "../engine/GameSnapshot.h"

// בדיקת עשן (smoke test) בלבד - איטרציה 9 דורשת שהיא תצייר לוח פשוט
// בלי לשנות את מצב המשחק. ספריית הציור עצמה עדיין לא נבחרה (ראו
// ImageView::render), אז אין כאן אימות פיקסלים - רק שהקריאה לא קורסת
// ושתמונת המצב שהתקבלה נשארת ללא שינוי.
TEST_CASE("ImageView - בדיקת עשן: מציירת לוח פשוט בלי לשנות את המצב") {
    GameSnapshot snap;
    snap.board_width  = 3;
    snap.board_height = 3;
    snap.game_over    = false;
    snap.pieces.push_back({ Chess::Kind::King, Chess::Color::White, {0, 0}, Chess::State::Idle });

    ImageView view;
    CHECK_NOTHROW(view.render(snap));

    CHECK(snap.board_width == 3);
    CHECK(snap.board_height == 3);
    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].kind == Chess::Kind::King);
    CHECK(snap.pieces[0].color == Chess::Color::White);
    CHECK(snap.pieces[0].cell == Position{ 0, 0 });
    CHECK_FALSE(snap.game_over);
}

TEST_CASE("ImageView - בדיקת עשן: לוח ריק לא קורס") {
    GameSnapshot snap;
    snap.board_width  = 2;
    snap.board_height = 2;
    snap.game_over    = false;

    ImageView view;
    CHECK_NOTHROW(view.render(snap));
}
