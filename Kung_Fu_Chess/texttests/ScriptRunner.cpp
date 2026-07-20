#include "ScriptRunner.h"
#include "ScriptParser.h"
#include "../server/io/BoardParser.h"
#include "../server/io/BoardPrinter.h"
#include "../server/engine/GameEngine.h"
#include "../server/app/logic/CommandDispatcher.h"
#include "../client/input/BoardMapper.h"
#include "../client/view/ViewConfig.h"
#include "../shared/input/ClickResolver.h"
#include <optional>
#include <sstream>

namespace {
    Message moveMessage(Position from, Position to) {
        Message m;
        m.type = MessageType::Move;
        m.payload = { {"from", {{"row", from.row}, {"col", from.col}}},
                      {"to",   {{"row", to.row},   {"col", to.col}}} };
        return m;
    }

    Message jumpMessage(Position pos) {
        Message m;
        m.type = MessageType::Jump;
        m.payload = { {"pos", {{"row", pos.row}, {"col", pos.col}}} };
        return m;
    }

    bool outOfBounds(const GameSnapshot& snap, int x, int y) {
        int boardWidthPx  = snap.board_width  * ViewConfig::CELL_SIZE;
        int boardHeightPx = snap.board_height * ViewConfig::CELL_SIZE;
        return x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx;
    }

    // The DSL has no concept of "which player" is acting - a script command
    // acts as whichever color already owns the piece being moved, so
    // CommandDispatcher's role check (added for real multiplayer) never
    // blocks these scripted scenarios.
    Chess::Color pieceColorAt(const GameSnapshot& snap, Position pos) {
        for (const auto& p : snap.pieces)
            if (p.cell == pos)
                return p.color;
        return Chess::Color::None;
    }
}

ScriptResult ScriptRunner::run(std::istream& input) {
    auto board = BoardParser::parseBoardOnly(input);
    GameEngine engine(board);
    std::optional<Position> selectedPos;

    auto commands = ScriptParser::parse(input, board->getHeight());

    for (const auto& cmd : commands) {
        switch (cmd.type) {
            case ScriptCommandType::Click: {
                GameSnapshot snap = engine.snapshot();
                if (outOfBounds(snap, cmd.x, cmd.y)) {
                    selectedPos = std::nullopt;
                    break;
                }

                Position clicked = BoardMapper::mapToPosition(cmd.x, cmd.y);
                ClickOutcome outcome = ClickResolver::resolve(snap, selectedPos, clicked);

                switch (outcome.type) {
                    case ClickOutcomeType::Selected:
                        selectedPos = outcome.pos;
                        break;
                    case ClickOutcomeType::SendMove:
                        CommandDispatcher::dispatch(moveMessage(outcome.from, outcome.to), engine,
                                                     pieceColorAt(snap, outcome.from));
                        selectedPos = std::nullopt;
                        break;
                    case ClickOutcomeType::SendJump:
                        CommandDispatcher::dispatch(jumpMessage(outcome.pos), engine,
                                                     pieceColorAt(snap, outcome.pos));
                        selectedPos = std::nullopt;
                        break;
                    case ClickOutcomeType::NoOp:
                        break;
                }
                break;
            }

            case ScriptCommandType::Jump: {
                GameSnapshot snap = engine.snapshot();
                if (outOfBounds(snap, cmd.x, cmd.y))
                    break;
                Position pos = BoardMapper::mapToPosition(cmd.x, cmd.y);
                CommandDispatcher::dispatch(jumpMessage(pos), engine, pieceColorAt(snap, pos));
                break;
            }

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
