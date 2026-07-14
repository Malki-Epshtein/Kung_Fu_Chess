#pragma once
#include "Piece.h"
#include <string>

bool isLegalTransition(Chess::State from, Chess::State to);

bool isBusyState(Chess::State state);
bool isRestingState(Chess::State state);

Chess::State stateFromString(const std::string& name);
std::string  stateToFolderName(Chess::State state);
