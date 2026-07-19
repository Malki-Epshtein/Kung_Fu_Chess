#include "../doctest.h"
#include "../client/view/assets/AnimationFrameSelector.h"

TEST_CASE("selectAnimationFrame - זמן חלוף 0 מחזיר תמיד את הפריים הראשון") {
    CHECK(selectAnimationFrame(0, 6, true, 5) == 0);
    CHECK(selectAnimationFrame(0, 6, false, 5) == 0);
}

TEST_CASE("selectAnimationFrame - אנימציה לא-לולאתית נעצרת בפריים האחרון") {
    // 6 פריימים לשנייה, 5 פריימים - האנימציה מסתיימת אחרי ~833 מ\"ש
    CHECK(selectAnimationFrame(500, 6, false, 5) == 3);
    CHECK(selectAnimationFrame(10000, 6, false, 5) == 4); // הרבה מעבר לסוף - נשאר על האחרון
}

TEST_CASE("selectAnimationFrame - אנימציה לולאתית חוזרת לפריים הראשון") {
    // 5 פריימים לשנייה, 5 פריימים -> מחזור מלא בדיוק ב-1000 מ\"ש
    CHECK(selectAnimationFrame(0, 5, true, 5) == 0);
    CHECK(selectAnimationFrame(1000, 5, true, 5) == 0); // מחזור אחד שלם - חוזר להתחלה
    CHECK(selectAnimationFrame(1200, 5, true, 5) == 1); // מחזור אחד + פריים אחד
}

TEST_CASE("selectAnimationFrame - frameCount 0 או שלילי מחזיר 0 בלי קריסה") {
    CHECK(selectAnimationFrame(500, 6, true, 0) == 0);
    CHECK(selectAnimationFrame(500, 6, true, -1) == 0);
}

TEST_CASE("selectAnimationFrame - framesPerSec 0 או שלילי מחזיר 0 בלי קריסה") {
    CHECK(selectAnimationFrame(500, 0, true, 5) == 0);
    CHECK(selectAnimationFrame(500, -1, true, 5) == 0);
}

TEST_CASE("selectAnimationFrame - זמן חלוף שלילי (מקרה קצה בתזמון) לעולם לא מחזיר אינדקס שלילי") {
    CHECK(selectAnimationFrame(-1, 6, true, 5) >= 0);
    CHECK(selectAnimationFrame(-500, 6, true, 5) >= 0);
    CHECK(selectAnimationFrame(-1, 6, false, 5) >= 0);
}
