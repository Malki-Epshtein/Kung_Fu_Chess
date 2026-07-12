#pragma once
#include <string>
#include <vector>

enum class ScriptCommandType { Click, Wait, PrintBoard };

struct ScriptCommand {
    ScriptCommandType        type;
    int                      x = 0;  // עבור Click
    int                      y = 0;  // עבור Click
    int                      ms = 0; // עבור Wait
    std::vector<std::string> expectedBoard; // עבור PrintBoard - השורות הצפויות שאחריו בסקריפט
};
