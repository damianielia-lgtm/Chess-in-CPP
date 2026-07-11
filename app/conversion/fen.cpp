#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <cctype>
#include <span>
#include "../core/encoding.h"
#include "fen.h"

const std::array<char, 4> castling_strings = {'K', 'Q', 'k', 'q'};

Position from_fen(const std::string& fen_string) {
     Position position;

     std::stringstream ss(fen_string);
     std::array<std::string, 6> fen_info;
     for (int i = 0; i < 6; ++i) {
          ss >> fen_info[i];
     }

     ss.clear();
     ss.str(fen_info[0]);
     std::string token;
     std::array<std::string, 8> fen_ranks;
     int index = 0;
     while (index < 8 && std::getline(ss, token, '/')) {
          fen_ranks[index++] = token;
     }

     std::array<int, 64> board;
     int square_index = 0;
     for (int rank = 7; rank >= 0; --rank) {
          for (char item : fen_ranks[rank]) {
               if (std::isdigit(static_cast<unsigned char>(item))) {
                    for (int i = 0; i < (item - '0'); i++) {
                         board[square_index] = 0;
                         square_index++;
                    }
               } else {
                    board[square_index] = encode_piece(item);
                    square_index++;
               }
          }
     }
     position.board = board;

     position.turn = (fen_info[1] == "w") ? 0 : 1;

     int castling_rights = 0;
     if (fen_info[2] != "-") {
          for (int i = 0; i <= 3; i++) {
               if (fen_info[2].contains(castling_strings[i])) {
                    castling_rights |= (1 << i);
               }
          }
     }
     position.castling_rights = castling_rights;

     position.en_passant_target = (fen_info[3] != "-") ? (fen_info[3][1] - '1') * 8 + (fen_info[3][0] - 'a') : -1;

     position.halfmove_clock = std::stoi(fen_info[4]);

     position.move_clock = std::stoi(fen_info[5]);

     for (square_index = 0; square_index <= 63; square_index++) {
          if (board[square_index] == 6) {
               position.white_king = square_index;
          } else if (board[square_index] == 14) {
               position.black_king = square_index;
          }
     }

     return position;
}

std::string to_fen(const Position& position) {
     std::string fen_string;

     std::string fen_board;
     int file_index;
     int empty_count;
     for (int rank = 56; rank >= 0; rank -= 8) {
          std::span<const int> row = std::span(position.board).subspan(rank, 8);
          file_index = 0;
          while (file_index <= 7) {
               if (row[file_index] != 0) {
                    fen_board += decode_piece(row[file_index]);
               } else {
                    empty_count = 1;
                    while (file_index < 7) {
                         file_index += 1;
                         if (row[file_index] == 0) {
                              empty_count += 1;
                              continue;
                         } else {
                              file_index -= 1;
                              break;
                         }
                    }
                    fen_board += empty_count + '0';
               }
               file_index += 1;
          }
          fen_board += '/';
     }
     fen_board.pop_back();
     fen_string += fen_board + " ";

     fen_string += (position.turn == 0) ? "w " : "b ";

     std::string fen_castling_rights;
     if (position.castling_rights == 0) {
          fen_castling_rights = "-";
     } else {
          for (int i = 0; i <= 3; i++) {
               if ((position.castling_rights >> i) & 1 != 0) {
                    fen_castling_rights += castling_strings[i];
               }
          }
     }
     fen_string += fen_castling_rights + " ";

     std::string fen_en_passant;
     if (position.en_passant_target == -1) {
          fen_en_passant = "-";
     } else {
          char file = (position.en_passant_target & 7) + 'a';
          char rank = ((position.en_passant_target >> 3) & 7) + '1';
          fen_en_passant = std::string() + file + rank;
     }
     fen_string += fen_en_passant + " ";

     fen_string += std::to_string(position.halfmove_clock) + " ";
     
     fen_string += std::to_string(position.move_clock);

     return fen_string;
}