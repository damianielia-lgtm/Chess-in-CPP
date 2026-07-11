#include <iostream>
#include <array>
#include <algorithm>
#include <string>
#include "../core/encoding.h"
#include "display.h"

void print_board(const std::array<int, 64>& board, const bool& flip) {
     std::array<char, 64> string_board;
     std::transform(board.begin(), board.end(), string_board.begin(), decode_piece);
     std::string files = flip ? "    h g f e d c b a    \n" : "    a b c d e f g h    \n";
     std::cout << files;
     std::cout << "   -----------------   \n";
     for (int rank = 7; rank >= 0; --rank) {
          int display_rank = flip ? 7 - rank : rank;
          std::cout << display_rank + 1 << " | ";
          for (int file = 0; file <= 7; file++) {
               int display_file = flip ? 7 - file : file;
               std::cout << string_board[display_rank * 8 + display_file] << " ";
          }
          std::cout << "| " << display_rank + 1 << "\n";
     }
     std::cout << "   -----------------   \n";
     std::cout << files;
}