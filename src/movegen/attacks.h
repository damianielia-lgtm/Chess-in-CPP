#pragma once

#include "../core/square.h"
#include "../core/position.h"
#include "../core/piece.h"
#include "../core/move.h"

bool is_attacked_square(
    const Position& position,
    const Square square, 
    const Color attacking_color
);
bool is_legal_move(Position& position, const Move move);
