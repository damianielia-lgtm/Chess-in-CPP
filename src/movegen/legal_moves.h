#pragma once

#include "../core/move_list.h"
#include "../core/position.h"

enum class MoveGeneration { All, Tactical };

void generate_all_moves(MovesList& legal_moves, Position& position, MoveGeneration loud);
MovesList all_moves(Position& position, MoveGeneration loud);
