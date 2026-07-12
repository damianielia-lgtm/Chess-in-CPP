#pragma once
#include <array>
#include "../core/encoding.h"

struct undo_storage {
     int capture;
     int castling_rights;
     int en_passant_target;
     int halfmove_clock;
     int move_clock;
     int white_king;
     int black_king;
};

void make_move(const int move, std::array<int, 64>& board, const int moving_piece);
void unmake_move(const int move, std::array<int, 64>& board, const int capture);
undo_storage apply_move(const int move, Position& position);
void take_back_move(const int move, Position& position, const undo_storage& undo_stack);