#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <cctype>
#include <span>
#include <algorithm>
#include <vector>
#include "../core/encoding.h"
#include "fen.h"
#include "../game_logic/legal_moves.h"

const std::string castling_strings = "KQkq";

struct fen {
     std::string board;
     std::string turn;
     std::string castling_rights;
     std::string en_passant_target;
     std::string halfmove_clock;
     std::string move_clock;
};

Position from_fen(const std::string& fen_string) {
     if (fen_string == "startpos") {
          return start_pos();
     }

     Position position;

     std::stringstream ss(fen_string);
     std::array<std::string, 6> collector;
     for (int i = 0; i < 6; ++i) {
          ss >> collector[i];
     }
     fen fen_info;
     fen_info.board = collector[0];
     fen_info.turn = collector[1];
     fen_info.castling_rights = collector[2];
     fen_info.en_passant_target = collector[3];
     fen_info.halfmove_clock = collector[4];
     fen_info.move_clock = collector[5];

     ss.clear();
     ss.str(fen_info.board);
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

     position.turn = (fen_info.turn == "w") ? 0 : 1;

     int castling_rights = 0;
     if (fen_info.castling_rights != "-") {
          for (int i = 0; i <= 3; i++) {
               if (fen_info.castling_rights.contains(castling_strings[i])) {
                    castling_rights |= (1 << i);
               }
          }
     }
     position.castling_rights = castling_rights;

     position.en_passant_target = (fen_info.en_passant_target != "-") ? (fen_info.en_passant_target[1] - '1') * 8 + (fen_info.en_passant_target[0] - 'a') : -1;

     position.halfmove_clock = std::stoi(fen_info.halfmove_clock);

     position.move_clock = std::stoi(fen_info.move_clock);

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

bool is_valid_fen(std::string fen_string) {
     if (fen_string == "startpos") {return true;}

     if (std::count(fen_string.begin(), fen_string.end(), ' ') != 5) {return false;}
     std::stringstream ss(fen_string);
     std::array<std::string, 6> collector;
     for (int i = 0; i < 6; ++i) {
          ss >> collector[i];
     }
     fen fen_info;
     fen_info.board = collector[0];
     fen_info.turn = collector[1];
     fen_info.castling_rights = collector[2];
     fen_info.en_passant_target = collector[3];
     fen_info.halfmove_clock = collector[4];
     fen_info.move_clock = collector[5];

     if (std::count(fen_info.board.begin(), fen_info.board.end(), '/') != 7) {return false;}
     if (fen_info.board.starts_with('/') || fen_info.board.ends_with('/')) {return false;}
     std::stringstream rowStream(fen_info.board);
     std::string row;
     int white_kings = 0;
     int black_kings = 0;
     while (std::getline(rowStream, row, '/')) {
          int square_count = 0;
          for (int i = 0; i < row.length(); i++) {
               char c = row[i];
               if (std::isdigit(static_cast<unsigned char>(c))) {
                    if (std::isdigit(static_cast<unsigned char>((row + ' ')[i + 1]))) {return false;}
                    int num = c - '0';
                    if (!((1 <= num) && (num <= 8))) {return false;}
                    square_count += num;
               } else if (std::string("pnbrqkPNBRQK").find(c) != std::string::npos) {
                    square_count++;
                    if (c == 'K') {white_kings++;}
                    if (c == 'k') {black_kings++;}
               } else {
                    return false;
               }
          }
          if (square_count != 8) {return false;}
     }
     if (white_kings != 1 || black_kings != 1) {return false;}

     if (fen_info.turn != "w" && fen_info.turn != "b") {return false;}

     if (fen_info.castling_rights != "-") {
          std::string valid_castling = "KQkq";
          for (char c : fen_info.castling_rights) {
               auto pos = valid_castling.find(c);
               if (pos == std::string::npos) {
                    return false;
               } else {
                    valid_castling.erase(pos, 1);
               }
          }
     }

     if (fen_info.en_passant_target != "-") {
          if (fen_info.en_passant_target.length() != 2) {return false;}
          if (fen_info.en_passant_target[0] < 'a' || fen_info.en_passant_target[0] > 'h') {return false;}
          if (fen_info.en_passant_target[1] != '3' && fen_info.en_passant_target[1] != '6') {return false;}
     }

     if (fen_info.halfmove_clock.length() > 6 || fen_info.halfmove_clock.empty() || std::ranges::any_of(fen_info.halfmove_clock, [](unsigned char c) {return !std::isdigit(c);})) {return false;}

     if (fen_info.move_clock.length() > 6 || fen_info.move_clock.empty() || std::ranges::any_of(fen_info.move_clock, [](unsigned char c) {return !std::isdigit(c);})) {return false;}
     if (std::stoi(fen_info.move_clock) == 0) {return false;}

     Position pos = from_fen(fen_string);

     for (int square_index = 0; square_index <= 7; square_index++) {
          if ((pos.board[square_index] & 7) == 1) {return false;}
     }
     for (int square_index = 56; square_index <= 63; square_index++) {
          if ((pos.board[square_index] & 7) == 1) {return false;}
     }

     int king = (!pos.turn) ? pos.black_king : pos.white_king;
     if (is_attacked_square(pos.board, king, pos.turn)) {return false;}

     if (pos.castling_rights & 1) {
          if (pos.board[4] != 6) {return false;}
          if (pos.board[7] != 4) {return false;}
     }
     if (pos.castling_rights & 2) {
          if (pos.board[4] != 6) {return false;}
          if (pos.board[0] != 4) {return false;}
     }
     if (pos.castling_rights & 4) {
          if (pos.board[60] != 14) {return false;}
          if (pos.board[63] != 12) {return false;}
     }
     if (pos.castling_rights & 8) {
          if (pos.board[60] != 14) {return false;}
          if (pos.board[56] != 12) {return false;}
     }

     if (pos.en_passant_target != -1) {
          if (pos.turn) {
               if ((40 <= pos.en_passant_target) && (pos.en_passant_target <= 47)) {return false;}
          } else {
               if ((16 <= pos.en_passant_target) && (pos.en_passant_target <= 23)) {return false;}
          }
          if (pos.board[pos.en_passant_target] != 0) {return false;}
          int rank_offset = (pos.turn) ? 8 : -8;
          if (pos.board[pos.en_passant_target + rank_offset] != ((!pos.turn << 3) | 1)) {return false;}
     }

     return true;
}