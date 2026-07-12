#pragma once
#include "ScriptResult.h"
#include <iostream>

// מריץ סקריפט DSL מלא (Board: + click/wait/print board) דרך ה-API הציבורי
// בלבד (Controller, GameEngine, BoardPrinter) - בלי לקרוא ישירות ל-Board,
// RuleEngine או RealTimeArbiter.
class ScriptRunner {
public:
    static ScriptResult run(std::istream& input);
};
