#pragma once
#include <array>
#include <string>

extern const int capture_flag;
extern const int double_pawn_flag;
extern const int en_passant_flag;
extern const int castling_flag;
extern const int promotion_flag;

int encode_piece(const char& piece);
char decode_piece(const int& piece);
int from_uci(const std::string& move);
std::string to_uci(const int& move);
std::array<int, 64> create_board();