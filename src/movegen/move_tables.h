#pragma once

#include <array>
#include <cassert>

#include "../core/piece.h"
#include "../core/square.h"

template<std::size_t maximum>
class Targets {
public:
    constexpr Targets(std::initializer_list<Square> init) {
        assert(init.size() <= maximum);
        for (auto target : init) {
            squares_[count_++] = target;
        }
    }

    constexpr void push(Square target) noexcept {
        assert(!full());
        squares_[count_++] = target;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return count_; }
    [[nodiscard]] constexpr bool full() const noexcept { return count_ == maximum; }
    [[nodiscard]] constexpr bool empty() const noexcept { return count_ == 0; }

    using const_iterator = std::array<Square, maximum>::const_iterator;
    constexpr const_iterator begin() const noexcept { return squares_.begin(); }
    constexpr const_iterator end() const noexcept { return squares_.begin() + count_; }

private:
    std::array<Square, maximum> squares_;
    std::size_t count_ = 0;
};

struct DoublePawnSquares {
    Square intermediate;
    Square target;
    bool available;
};

struct MoveTables {
    std::array<std::array<Targets<2>, 64>, 2> PAWN_ATTACKS;
    std::array<std::array<Square, 64>, 2> PAWN_PUSH;
    std::array<std::array<DoublePawnSquares, 64>, 2> DOUBLE_PAWN_PUSH;
    std::array<Targets<8>, 64> KNIGHT_MOVEMENT;
    std::array<std::array<Targets<7>, 4>, 64> BISHOP_MOVEMENT;
    std::array<std::array<Targets<7>, 4>, 64> ROOK_MOVEMENT;
    std::array<Targets<8>, 64> KING_MOVEMENT;
};

constexpr bool in_board(int rank, int file) {
return (0 <= rank && rank <= 7 && 0 <= file && file <= 7);
}

constexpr std::array<std::array<int, 2>, 8> knight_offsets = {{{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};
constexpr std::array<std::array<int, 2>, 8> king_offsets = {{{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}}};
constexpr std::array<std::array<int, 2>, 4> bishop_offsets = {{{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}};
constexpr std::array<std::array<int, 2>, 4> rook_offsets = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

constexpr MoveTables precompute_movement() {
    MoveTables table{};
    for (int square_index = 0; square_index <= 63; square_index++) {
        int rank = square_index / 8;
        int file = square_index % 8;
        if (in_board(rank + 1, file + 1)) {
            table.PAWN_ATTACKS[0][square_index].push(Square(square_index + 9));
        }
        if (in_board(rank + 1, file - 1)) {
            table.PAWN_ATTACKS[0][square_index].push(Square(square_index + 7));
        }
        if (in_board(rank - 1, file + 1)) {
            table.PAWN_ATTACKS[1][square_index].push(Square(square_index - 7));
        }
        if (in_board(rank - 1, file - 1)) {
            table.PAWN_ATTACKS[1][square_index].push(Square(square_index - 9));
        }

        if (in_board(rank + 1, file)) {
            table.PAWN_PUSH[0][square_index] = Square(square_index + 8);
        }
        if (in_board(rank - 1, file)) {
            table.PAWN_PUSH[1][square_index] = Square(square_index - 8);
        }

        if (rank == 1) {
            table.DOUBLE_PAWN_PUSH[0][square_index].intermediate = Square(square_index + 8);
            table.DOUBLE_PAWN_PUSH[0][square_index].target = Square(square_index + 16);
            table.DOUBLE_PAWN_PUSH[0][square_index].available = true;
        }
        if (rank == 6) {
            table.DOUBLE_PAWN_PUSH[1][square_index].intermediate = Square(square_index - 8);
            table.DOUBLE_PAWN_PUSH[1][square_index].target = Square(square_index - 16);
            table.DOUBLE_PAWN_PUSH[1][square_index].available = true;
        }

        int target_rank;
        int target_file;
        for (const auto& [rank_offset, file_offset] : knight_offsets) {
            target_rank = rank + rank_offset;
            target_file = file + file_offset;
            if (in_board(target_rank, target_file)) {
                table.KNIGHT_MOVEMENT[square_index].push(Square(target_rank * 8 + target_file));
            }
        }
        for (const auto& [rank_offset, file_offset] : king_offsets) {
            target_rank = rank + rank_offset;
            target_file = file + file_offset;
            if (in_board(target_rank, target_file)) {
                table.KING_MOVEMENT[square_index].push(Square(target_rank * 8 + target_file));
            }
        }

        for (std::uint8_t direction = 0; direction <= 3; direction++) {
            target_rank = rank;
            target_file = file;
            while (true) {
                target_rank += bishop_offsets[direction][0];
                target_file += bishop_offsets[direction][1];
                if (in_board(target_rank, target_file)) {
                        table.BISHOP_MOVEMENT[square_index][direction].push(Square(target_rank * 8 + target_file));
                        continue;
                } break;
            }
        }
        for (std::uint8_t direction = 0; direction <= 3; direction++) {
            target_rank = rank;
            target_file = file;
            while (true) {
                target_rank += rook_offsets[direction][0];
                target_file += rook_offsets[direction][1];
                if (in_board(target_rank, target_file)) {
                        table.ROOK_MOVEMENT[square_index][direction].push(Square(target_rank * 8 + target_file));
                        continue;
                } break;
            }
        }
    }
    
    return table;
}

inline constexpr MoveTables TABLES = precompute_movement();
