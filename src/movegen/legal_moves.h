#pragma once

#include "../core/move_list.h"
#include "../core/position.h"

void generate_all_moves(MovesList& legal_moves, Position& position, bool loud);
MovesList all_moves(Position& position, bool loud);
