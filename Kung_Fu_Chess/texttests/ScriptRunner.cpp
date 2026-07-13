#include "ScriptRunner.h"
#include "ScriptParser.h"
#include "../io/BoardParser.h"
#include "../io/BoardPrinter.h"
#include "../engine/GameEngine.h"
#include "../input/Controller.h"
#include <sstream>

ScriptResult ScriptRunner::run(std::istream& input) {
    auto board = BoardParser::parseBoardOnly(input);
    GameEngine engine(board);
    Controller controller(engine);

    auto commands = ScriptParser::parse(input, board->getHeight());

    for (const auto& cmd : commands) {
        switch (cmd.type) {
            case ScriptCommandType::Click:
                controller.handleMouseClick(cmd.x, cmd.y);
                break;

            case ScriptCommandType::Jump:
                controller.handleJump(cmd.x, cmd.y);
                break;

            case ScriptCommandType::Wait:
                engine.wait(cmd.ms);
                break;

            case ScriptCommandType::PrintBoard: {
                std::ostringstream actualOut;
                BoardPrinter::print(engine.snapshot(), actualOut);

                std::ostringstream expectedOut;
                for (const auto& line : cmd.expectedBoard)
                    expectedOut << line << "\n";

                if (actualOut.str() != expectedOut.str()) {
                    std::ostringstream msg;
                    msg << "expected:\n" << expectedOut.str()
                        << "actual:\n"   << actualOut.str();
                    return { false, msg.str() };
                }
                break;
            }
        }
    }

    return { true, "" };
}
