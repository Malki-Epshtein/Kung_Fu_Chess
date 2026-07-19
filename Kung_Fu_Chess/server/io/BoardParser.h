#pragma once
#include "../../shared/model/Board.h"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

class BoardParser {
public:
    static std::shared_ptr<Board> parseBoardOnly(std::istream& input);

private:
    static std::vector<std::string> splitLine(const std::string& line);
    static bool isValidToken(const std::string& token);
    static std::shared_ptr<Piece> createPiece(const std::string& token, Position pos);
};
