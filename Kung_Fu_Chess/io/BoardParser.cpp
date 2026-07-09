#include "BoardParser.h"
#include <sstream>
#include <stdexcept>

class ConcretePiece : public Piece {
public:
    ConcretePiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
        : Piece(id, color, kind, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

std::shared_ptr<Board> BoardParser::parseBoardOnly(std::istream& input) {
    std::vector<std::vector<std::string>> gridTokens;
    std::string line;
    size_t expectedWidth = 0;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> tokens = splitLine(line);
        if (tokens.empty()) continue;
        if (tokens[0] == "Commands:") break;
        if (tokens[0] == "Board:") continue;

        if (!gridTokens.empty() && tokens.size() != expectedWidth)
            throw std::runtime_error("ERROR ROW_WIDTH_MISMATCH");
        if (gridTokens.empty()) expectedWidth = tokens.size();

        for (const auto& t : tokens)
            if (!isValidToken(t)) throw std::runtime_error("ERROR UNKNOWN_TOKEN");

        gridTokens.push_back(tokens);
    }

    if (gridTokens.empty()) throw std::runtime_error("ERROR EMPTY_BOARD");

    int height = (int)gridTokens.size();
    int width  = (int)gridTokens[0].size();
    auto board = std::make_shared<Board>(width, height);

    for (int r = 0; r < height; ++r)
        for (int c = 0; c < width; ++c)
            if (gridTokens[r][c] != ".")
                board->addPiece(createPiece(gridTokens[r][c], {r, c}), {r, c});

    return board;
}

std::shared_ptr<Piece> BoardParser::createPiece(const std::string& token, Position pos) {
    Chess::Color color = (token[0] == 'w') ? Chess::Color::White : Chess::Color::Black;
    static int idCounter = 1;
    Chess::Kind kind;
    switch (token[1]) {
        case 'P': kind = Chess::Kind::Pawn;   break;
        case 'R': kind = Chess::Kind::Rook;   break;
        case 'N': kind = Chess::Kind::Knight; break;
        case 'B': kind = Chess::Kind::Bishop; break;
        case 'Q': kind = Chess::Kind::Queen;  break;
        case 'K': kind = Chess::Kind::King;   break;
        default: throw std::runtime_error("ERROR UNKNOWN_TOKEN");
    }
    return std::make_shared<ConcretePiece>(idCounter++, color, kind, pos);
}

bool BoardParser::isValidToken(const std::string& t) {
    if (t == ".") return true;
    if (t.length() != 2) return false;
    bool validColor = (t[0] == 'w' || t[0] == 'b');
    bool validType  = (t[1] == 'K' || t[1] == 'Q' || t[1] == 'R' ||
                       t[1] == 'B' || t[1] == 'N' || t[1] == 'P');
    return validColor && validType;
}

std::vector<std::string> BoardParser::splitLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while (ss >> token) tokens.push_back(token);
    return tokens;
}
