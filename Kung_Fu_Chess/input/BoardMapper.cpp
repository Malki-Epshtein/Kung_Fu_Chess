#include "BoardMapper.h"

Position BoardMapper::mapToPosition(int x, int y) {
    return Position{ y / CELL_SIZE, x / CELL_SIZE };
}