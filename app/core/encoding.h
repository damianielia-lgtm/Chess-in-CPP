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

struct Position {
    std::array<int, 64> board;
    int turn;
    int castling_rights;
    int en_passant_target;
    int halfmove_clock;
    int move_clock;
    int white_king;
    int black_king;
};
Position start_pos(const std::string* data = {});