#pragma once
#include "move_precomputing.h"

bool is_attacked_square(const std::array<int, 64>& board, const int square, const int attacking_color);