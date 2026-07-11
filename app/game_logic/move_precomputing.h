#pragma once
#include <array>

template<int maximum>
struct targets_arr {
    std::array<int, maximum> squares;
    int count;
};

struct double_pawn_squares {
    int intermediate;
    int target;
    bool available;
};

struct move_tables {
     std::array<std::array<targets_arr<2>, 64>, 2> PAWN_ATTACKS;
     std::array<std::array<int, 64>, 2> PAWN_PUSH;
     std::array<std::array<double_pawn_squares, 64>, 2> DOUBLE_PAWN_PUSH;
     std::array<targets_arr<8>, 64> KNIGHT_MOVEMENT;
     std::array<std::array<targets_arr<7>, 4>, 64> BISHOP_MOVEMENT;
     std::array<std::array<targets_arr<7>, 4>, 64> ROOK_MOVEMENT;
     std::array<targets_arr<8>, 64> KING_MOVEMENT;
};

template<int maximum>
constexpr void add_target(targets_arr<maximum>& arr, int target) {
     arr.squares[arr.count] = target;
     arr.count++;
}

constexpr bool in_board(int rank, int file) {
     return (0 <= rank && rank <= 7 && 0 <= file && file <= 7);
}

constexpr std::array<std::array<int, 2>, 8> knight_offsets = {{{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};
constexpr std::array<std::array<int, 2>, 8> king_offsets = {{{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}}};
constexpr std::array<std::array<int, 2>, 4> bishop_offsets = {{{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}};
constexpr std::array<std::array<int, 2>, 4> rook_offsets = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

constexpr move_tables precompute_movement() {
     move_tables table{};
     int rank;
     int file;
     int target_rank;
     int target_file;
     for (int square_index = 0; square_index <= 63; square_index++) {
          rank = square_index / 8;
          file = square_index % 8;
          if (in_board(rank + 1, file + 1)) {
               add_target(table.PAWN_ATTACKS[0][square_index], square_index + 9);
          }
          if (in_board(rank + 1, file - 1)) {
               add_target(table.PAWN_ATTACKS[0][square_index], square_index + 7);
          }
          if (in_board(rank - 1, file + 1)) {
               add_target(table.PAWN_ATTACKS[1][square_index], square_index - 7);
          }
          if (in_board(rank - 1, file - 1)) {
               add_target(table.PAWN_ATTACKS[1][square_index], square_index - 9);
          }

          if (in_board(rank + 1, file)) {
               table.PAWN_PUSH[0][square_index] = square_index + 8;
          }
          if (in_board(rank - 1, file)) {
               table.PAWN_PUSH[1][square_index] = square_index - 8;
          }

          if (rank == 1) {
               table.DOUBLE_PAWN_PUSH[0][square_index].intermediate = square_index + 8;
               table.DOUBLE_PAWN_PUSH[0][square_index].target = square_index + 16;
               table.DOUBLE_PAWN_PUSH[0][square_index].available = true;
          }
          if (rank == 6) {
               table.DOUBLE_PAWN_PUSH[1][square_index].intermediate = square_index - 8;
               table.DOUBLE_PAWN_PUSH[1][square_index].target = square_index - 16;
               table.DOUBLE_PAWN_PUSH[1][square_index].available = true;
          }

          for (const auto& [rank_offset, file_offset] : knight_offsets) {
               target_rank = rank + rank_offset;
               target_file = file + file_offset;
               if (in_board(target_rank, target_file)) {
                    add_target(table.KNIGHT_MOVEMENT[square_index], (target_rank * 8 + target_file));
               }
          }
          for (const auto& [rank_offset, file_offset] : king_offsets) {
               target_rank = rank + rank_offset;
               target_file = file + file_offset;
               if (in_board(target_rank, target_file)) {
                    add_target(table.KING_MOVEMENT[square_index], (target_rank * 8 + target_file));
               }
          }

          for (int direction = 0; direction <= 3; direction++) {
               target_rank = rank;
               target_file = file;
               while (true) {
                    target_rank += bishop_offsets[direction][0];
                    target_file += bishop_offsets[direction][1];
                    if (in_board(target_rank, target_file)) {
                         add_target(table.BISHOP_MOVEMENT[square_index][direction], (target_rank * 8 + target_file));
                         continue;
                    } break;
               }
          }
          for (int direction = 0; direction <= 3; direction++) {
               target_rank = rank;
               target_file = file;
               while (true) {
                    target_rank += rook_offsets[direction][0];
                    target_file += rook_offsets[direction][1];
                    if (in_board(target_rank, target_file)) {
                         add_target(table.ROOK_MOVEMENT[square_index][direction], (target_rank * 8 + target_file));
                         continue;
                    } break;
               }
          }
     }
     return table;
}

inline constexpr move_tables TABLES = precompute_movement();