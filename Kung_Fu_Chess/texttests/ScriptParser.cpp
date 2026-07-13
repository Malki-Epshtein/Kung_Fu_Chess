#include "ScriptParser.h"
#include <sstream>
#include <stdexcept>

std::vector<ScriptCommand> ScriptParser::parse(std::istream& input, int boardHeight) {
    std::vector<ScriptCommand> commands;
    std::string line;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream ls(line);
        std::string word;
        ls >> word;

        if (word == "click") {
            ScriptCommand cmd;
            cmd.type = ScriptCommandType::Click;
            ls >> cmd.x >> cmd.y;
            commands.push_back(cmd);
        }
        else if (word == "jump") {
            ScriptCommand cmd;
            cmd.type = ScriptCommandType::Jump;
            ls >> cmd.x >> cmd.y;
            commands.push_back(cmd);
        }
        else if (word == "wait") {
            ScriptCommand cmd;
            cmd.type = ScriptCommandType::Wait;
            ls >> cmd.ms;
            commands.push_back(cmd);
        }
        else if (word == "print") {
            std::string second;
            ls >> second;
            if (second != "board") throw std::runtime_error("ERROR UNKNOWN_COMMAND");

            ScriptCommand cmd;
            cmd.type = ScriptCommandType::PrintBoard;
            for (int i = 0; i < boardHeight; ++i) {
                if (!std::getline(input, line))
                    throw std::runtime_error("ERROR MISSING_EXPECTED_BOARD");
                if (!line.empty() && line.back() == '\r') line.pop_back();
                cmd.expectedBoard.push_back(line);
            }
            commands.push_back(cmd);
        }
        else {
            throw std::runtime_error("ERROR UNKNOWN_COMMAND");
        }
    }

    return commands;
}
