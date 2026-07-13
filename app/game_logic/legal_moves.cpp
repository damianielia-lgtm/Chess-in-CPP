#include <array>
#include "move_precomputing.h"
#include "../core/encoding.h"
#include "legal_moves.h"
#include "../core/move_simulation.h"

targets_arr<32> pseudo_legal_moves(int square_index, int piece_moved, const Position& position, bool loud) {
     targets_arr<32> moves{};
     const std::array<int, 64>& board = position.board;
     const int& turn = position.turn;
     int target_index;
     int piece;
     piece_moved &= 7;
     if (piece_moved == 1) {
          const targets_arr<2>& pawn_captures = TABLES.PAWN_ATTACKS[turn][square_index];
          for (int indexes = 0; indexes < pawn_captures.count; indexes++) {
               target_index = pawn_captures.squares[indexes];
               piece = board[target_index];
               if (target_index == position.en_passant_target) {
                    add_target(moves, square_index | (target_index << 6) | capture_flag | en_passant_flag);
               } else if ((piece != 0) && ((piece >> 3) != turn)) {
                    if (7 >= target_index || target_index >= 56) {
                         for (int promotion = 2; promotion <= 5; promotion++) {
                              add_target(moves, square_index | (target_index << 6) | (promotion << 12) | capture_flag | promotion_flag);
                         }
                    } else {
                         add_target(moves, square_index | (target_index << 6) | capture_flag);
                    }
               }
          }
          target_index = TABLES.PAWN_PUSH[turn][square_index];
          if (board[target_index] == 0) {
               if (7 >= target_index || target_index >= 56) {
                    for (int promotion = 2; promotion <= 5; promotion++) {
                         add_target(moves, square_index | (target_index << 6) | (promotion << 12) | promotion_flag);
                    }
               } else {
                    if (!loud) {add_target(moves, square_index | (target_index << 6));}
               }
          }
          const double_pawn_squares& double_pawn_push = TABLES.DOUBLE_PAWN_PUSH[turn][square_index];
          if (double_pawn_push.available && board[double_pawn_push.intermediate] == 0 && board[double_pawn_push.target] == 0) {
               if (!loud) {add_target(moves, square_index | (double_pawn_push.target << 6) | double_pawn_flag);}
          }
     }
     if (piece_moved == 2) {
          const targets_arr<8>& knight_movement = TABLES.KNIGHT_MOVEMENT[square_index];
          for (int indexes = 0; indexes < knight_movement.count; indexes++) {
               target_index = knight_movement.squares[indexes];
               piece = board[target_index];
               if (piece == 0) {
                    if (!loud) {add_target(moves, square_index | (target_index << 6));}
               } else if ((piece >> 3) != turn) {
                    add_target(moves, square_index | (target_index << 6) | capture_flag);
               }
          }
     }
     if (piece_moved == 6) {
          const targets_arr<8>& king_movement = TABLES.KING_MOVEMENT[square_index];
          for (int indexes = 0; indexes < king_movement.count; indexes++) {
               target_index = king_movement.squares[indexes];
               piece = board[target_index];
               if (piece == 0) {
                    if (!loud) {add_target(moves, square_index | (target_index << 6));}
               } else if ((piece >> 3) != turn) {
                    add_target(moves, square_index | (target_index << 6) | capture_flag);
               }
          }
     }
     if (piece_moved == 3 || piece_moved == 5) {
          for (const targets_arr<7>& bishop_ray : TABLES.BISHOP_MOVEMENT[square_index]) {
               for (int indexes = 0; indexes < bishop_ray.count; indexes++) {
                    target_index = bishop_ray.squares[indexes];
                    piece = board[target_index];
                    if (piece == 0) {
                         if (!loud) {add_target(moves, square_index | (target_index << 6));}
                         continue;
                    }
                    else if ((piece >> 3) != turn) {
                         add_target(moves, square_index | (target_index << 6) | capture_flag);
                         break;
                    } else {break;}
               }
          }
     }
     if (piece_moved == 4 || piece_moved == 5) {
          for (const targets_arr<7>& rook_ray : TABLES.ROOK_MOVEMENT[square_index]) {
               for (int indexes = 0; indexes < rook_ray.count; indexes++) {
                    target_index = rook_ray.squares[indexes];
                    piece = board[target_index];
                    if (piece == 0) {
                         if (!loud) {add_target(moves, square_index | (target_index << 6));}
                         continue;
                    }
                    else if ((piece >> 3) != turn) {
                         add_target(moves, square_index | (target_index << 6) | capture_flag);
                         break;
                    } else {break;}
               }
          }
     }
     return moves;
}

bool is_attacked_square(const std::array<int, 64>& board, const int& square, const int& attacking_color) {
     int piece;
     const int color_offset = attacking_color << 3;
     const int attacking_pawn = color_offset | 1;
     const int attacking_knight = color_offset | 2;
     const int attacking_bishop = color_offset | 3;
     const int attacking_rook = color_offset | 4;
     const int attacking_queen = color_offset | 5;
     const int attacking_king = color_offset | 6;

     const targets_arr<2>& pawn_attacks = TABLES.PAWN_ATTACKS[attacking_color ^ 1][square];
     for (int indexes = 0; indexes < pawn_attacks.count; indexes++) {
          if (board[pawn_attacks.squares[indexes]] == attacking_pawn) {return true;}
     }
     const targets_arr<8>& knight_attacks = TABLES.KNIGHT_MOVEMENT[square];
     for (int indexes = 0; indexes < knight_attacks.count; indexes++) {
          if (board[knight_attacks.squares[indexes]] == attacking_knight) {return true;}
     }
     const targets_arr<8>& king_attacks = TABLES.KING_MOVEMENT[square];
     for (int indexes = 0; indexes < king_attacks.count; indexes++) {
          if (board[king_attacks.squares[indexes]] == attacking_king) {return true;}
     }

     for (const targets_arr<7>& bishop_ray : TABLES.BISHOP_MOVEMENT[square]) {
          for (int indexes = 0; indexes < bishop_ray.count; indexes++) {
               piece = board[bishop_ray.squares[indexes]];
               if (piece == attacking_bishop || piece == attacking_queen) {return true;}
               if (piece != 0) {break;}
          }
     }
     for (const targets_arr<7>& rook_ray : TABLES.ROOK_MOVEMENT[square]) {
          for (int indexes = 0; indexes < rook_ray.count; indexes++) {
               piece = board[rook_ray.squares[indexes]];
               if (piece == attacking_rook || piece == attacking_queen) {return true;}
               if (piece != 0) {break;}
          }
     }
     return false;
}

bool has_legal_moves(Position& position) {
     std::array<int, 64>& board = position.board;
     const int turn = position.turn;
     for (int square_index = 0; square_index <= 63; square_index++) {
          int piece = board[square_index];
          if (piece != 0 && (piece >> 3) == turn) {
               targets_arr<32> pseudo_moves = pseudo_legal_moves(square_index, piece, position, false);
               for (int move_index = 0; move_index < pseudo_moves.count; move_index++) {
                    if (is_legal_move(position, pseudo_moves.squares[move_index], piece)) {return true;}
               }
          }
     }
     return false;
}

bool is_legal_move(Position& position, const int move, const int piece_moved) {
     std::array<int, 64>& board = position.board;
     const int turn = position.turn;
     const int target_square = (move >> 6) & 63;
     const int capture = (move & en_passant_flag) ? board[(move & 56) | (target_square & 7)] : board[target_square];
     make_move(move, board, piece_moved);
     const int king = ((piece_moved & 7) == 6) ? target_square : (turn) ? position.black_king : position.white_king;
     if (is_attacked_square(board, king, turn ^ 1)) {
          unmake_move(move, board, capture);
          return false;
     }
     unmake_move(move, board, capture);
     return true;
}