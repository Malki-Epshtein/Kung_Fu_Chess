#pragma once
#include "../../../shared/model/Board.h"
#include <memory>

// The standard starting position, used to seed every new room (the
// startup default room in server_main.cpp, and any room allocated later
// via AllocateRoomHandler - for a regular room or a PLAY match alike).
std::shared_ptr<Board> makeStartingBoard();
