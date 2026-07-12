#include <array>
#include "move_precomputing.h"

int piece;
bool is_attacked_square(const std::array<int, 64>& board, const int square, const int attacking_color) {
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
