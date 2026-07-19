#include "BoardMapper.h"
#include "../view/ViewConfig.h"

Position BoardMapper::mapToPosition(int x, int y) {
    return Position{ y / ViewConfig::CELL_SIZE, x / ViewConfig::CELL_SIZE };
}
