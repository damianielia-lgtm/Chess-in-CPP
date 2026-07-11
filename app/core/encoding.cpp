#include <array>
#include <string>
#include <cctype>
#include "encoding.h"

const int capture_flag = 1 << 15;
const int double_pawn_flag = 1 << 16;
const int en_passant_flag = 1 << 17;
const int castling_flag = 1 << 18;
const int promotion_flag = 1 << 19;

int encode_piece(const char& piece) {
     switch (piece) {
          case 'P': return 1;
          case 'N': return 2;
          case 'B': return 3;
          case 'R': return 4;
          case 'Q': return 5;
          case 'K': return 6;
          
          case 'p': return 9;
          case 'n': return 10;
          case 'b': return 11;
          case 'r': return 12;
          case 'q': return 13;
          case 'k': return 14;
          
          default: return 0;
     }    
}

char decode_piece(const int& piece) {
     switch (piece) {
          case 1: return 'P';
          case 2: return 'N';
          case 3: return 'B';
          case 4: return 'R';
          case 5: return 'Q';
          case 6: return 'K';

          case 9: return 'p';
          case 10: return 'n';
          case 11: return 'b';
          case 12: return 'r';
          case 13: return 'q';
          case 14: return 'k';
          
          default: return '.';
     }
}

int from_uci(const std::string& move) {
     int origin_square = (move[1] - '1') * 8 + (move[0] - 'a');
     int target_square = (move[3] - '1') * 8 + (move[2] - 'a');
     int promotion = (move.length() > 4) ? encode_piece(move[4]) & 7 : 0;

     return origin_square | (target_square << 6) | (promotion << 12);
}

std::string to_uci(const int& move) {
     char origin_file = (move & 7) + 'a';
     char origin_rank = ((move >> 3) & 7) + '1';
     char target_file = ((move >> 6) & 7) + 'a';
     char target_rank = ((move >> 9) & 7) + '1';

     std::string uci = std::string() + origin_file + origin_rank + target_file + target_rank;

     int promotion = (move >> 12) & 7;
     if (promotion) {
          uci += char(decode_piece(promotion | 8));
     }

     return uci;
}

std::array<int, 64> create_board() {
     std::array<int, 64> board = {
        4, 2, 3, 5, 6, 3, 2, 4,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        9, 9, 9, 9, 9, 9, 9, 9,
        12, 10, 11, 13, 14, 11, 10, 12};

     return board;
}