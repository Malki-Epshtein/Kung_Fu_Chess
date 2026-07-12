#pragma once
#include "ScriptCommand.h"
#include <iostream>
#include <vector>

// מפרש את שורות הפקודות (click / wait / print board) בלבד.
// הפרשנות של חלק ה-Board נעשית על ידי BoardParser, לפני קריאה ל-parse.
class ScriptParser {
public:
    static std::vector<ScriptCommand> parse(std::istream& input, int boardHeight);
};
