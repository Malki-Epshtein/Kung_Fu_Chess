#pragma once
#include "../../shared/model/Board.h"
#include <memory>

// The standard starting position, used to seed every new room (the
// startup default room in server_main.cpp, and any room created later via
// a CreateRoom message in WsServer.cpp).
std::shared_ptr<Board> makeStartingBoard();
