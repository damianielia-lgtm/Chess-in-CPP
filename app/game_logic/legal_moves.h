#pragma once
#include "move_precomputing.h"
#include "../core/encoding.h"

bool is_attacked_square(const std::array<int, 64>& board, const int& square, const int& attacking_color);
targets_arr<32> pseudo_legal_moves(int square_index, int piece_moved, const Position& position, bool loud);
bool is_legal_move(Position& position, const int move, const int piece_moved);