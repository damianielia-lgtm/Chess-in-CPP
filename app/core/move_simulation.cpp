#include <array>
#include "../core/encoding.h"
#include "move_simulation.h"

std::array<int, 3> castling_rook_movement(int move) {
     switch(move) {
          case 388 | (1 << 18): return {7, 5, 4};
          case 132 | (1 << 18): return {0, 3, 4};
          case 4028 | (1 << 18): return {63, 61, 12};
          case 3772 | (1 << 18): return {56, 59, 12};
          default: return {-1, -1, -1};
     }
}

void make_move(const int move, std::array<int, 64>& board, const int moving_piece) {
     int move_origin = move & 63;
     int move_target = (move >> 6) & 63;
     int promotion = (move >> 12) & 7;
     if (promotion != 0) {board[move_target] = (moving_piece > 7) ? promotion | 8 : promotion;}
     else {board[move_target] = moving_piece;}
     board[move_origin] = 0;
     if (move & en_passant_flag) {board[(move_origin & 56) | (move_target & 7)] = 0;}
     if (move & castling_flag) {
          std::array<int, 3> rook_movement = castling_rook_movement(move);
          board[rook_movement[0]] = 0;
          board[rook_movement[1]] = rook_movement[2];
     }
}

void unmake_move(const int move, std::array<int, 64>& board, const int capture) {
     int move_origin = move & 63;
     int move_target = (move >> 6) & 63;
     if (move & promotion_flag) {board[move_origin] = (move_target >= 56) ? 1 : 9;}
     else {board[move_origin] = board[move_target];}
     if (move & en_passant_flag) {
        board[(move_origin & 56) | (move_target & 7)] = capture;
        board[move_target] = 0;
     } else {board[move_target] = capture;}
     if (move & castling_flag) {
          std::array<int, 3> rook_movement = castling_rook_movement(move);
          board[rook_movement[1]] = 0;
          board[rook_movement[0]] = rook_movement[2];
     }
}

undo_storage apply_move(const int move, Position& position) {
     std::array<int, 64>& board = position.board;
     int move_origin = move & 63;
     int move_target = (move >> 6) & 63;
     int moving_piece = board[move_origin];
     undo_storage undo_stack{};
     undo_stack.capture = (move & en_passant_flag) ? board[(move_origin & 56) | (move_target & 7)] : board[move_target];
     undo_stack.castling_rights = position.castling_rights;
     undo_stack.en_passant_target = position.en_passant_target;
     undo_stack.halfmove_clock = position.halfmove_clock;
     undo_stack.move_clock = position.move_clock;
     undo_stack.white_king = position.white_king;
     undo_stack.black_king = position.black_king;
     make_move(move, position.board, moving_piece);
     position.turn ^= 1;
     if (move_origin == 7 || move_origin == 4 || move_target == 7) {position.castling_rights &= 14;}
     if (move_origin == 0 || move_origin == 4 || move_target == 0) {position.castling_rights &= 13;}
     if (move_origin == 63 || move_origin == 60 || move_target == 63) {position.castling_rights &= 11;}
     if (move_origin == 56 || move_origin == 60 || move_target == 56) {position.castling_rights &= 7;}
     position.en_passant_target = (move & double_pawn_flag) ? (move_origin + move_target) / 2 : -1;
     if (moving_piece == 1 || moving_piece == 9 || move & capture_flag) {position.halfmove_clock = 0;}
     else {position.halfmove_clock++;}
     if (position.turn == 0) {position.move_clock++;}
     if (moving_piece == 6) {position.white_king = move_target;}
     else if (moving_piece == 14) {position.black_king = move_target;}
     return undo_stack;
}

void take_back_move(const int move, Position& position, const undo_storage& undo_stack) {
     unmake_move(move, position.board, undo_stack.capture);
     position.turn ^= 1;
     position.castling_rights = undo_stack.castling_rights;
     position.en_passant_target = undo_stack.en_passant_target;
     position.halfmove_clock = undo_stack.halfmove_clock;
     position.move_clock = undo_stack.move_clock;
     position.white_king = undo_stack.white_king;
     position.black_king = undo_stack.black_king;
}