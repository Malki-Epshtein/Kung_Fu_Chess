#pragma once
#include "ScriptResult.h"
#include <iostream>

// מריץ סקריפט DSL מלא (Board: + click/wait/print board) דרך ה-API הציבורי
// בלבד (ClickResolver, CommandDispatcher, GameEngine, BoardPrinter) - בלי
// לקרוא ישירות ל-Board, RuleEngine או RealTimeArbiter. משתמש באותה לוגיקת
// "קליק ראשון/שני" בדיוק כמו הלקוח האמיתי (Controller), דרך ClickResolver
// המשותף - לא מדובר ב-Controller עצמו כי הוא הפך תלוי-רשת (Stage C4).
class ScriptRunner {
public:
    static ScriptResult run(std::istream& input);
};
