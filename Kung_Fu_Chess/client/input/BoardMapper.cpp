#include "BoardMapper.h"

Position BoardMapper::mapToPosition(int x, int y, int cellSize) {
    return Position{ y / cellSize, x / cellSize };
}
