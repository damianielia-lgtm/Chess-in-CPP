#pragma once

#include <array>
#include <cassert>
#include <cstddef>

#include "../core/move.h"
#include "../core/position.h"

constexpr std::size_t max_ply = 16;

class MovesList {
public:
    static constexpr std::size_t capacity = 256;

    MovesList() noexcept = default;

    void push(Move move) noexcept {
        assert(!full());
        moves_[count_++] = move;
    }

    std::size_t size() const noexcept { return count_; }
    bool full() const noexcept { return count_ == capacity; }
    bool empty() const noexcept { return count_ == 0; }
    void clear() noexcept { count_ = 0; }

    using const_iterator = std::array<Move, capacity>::const_iterator;
    const_iterator begin() const { return moves_.begin(); }
    const_iterator end() const { return moves_.begin() + count_; }

private:
    std::array<Move, capacity> moves_{};
    std::size_t count_ = 0;
};

void generate_all_moves(MovesList& legal_moves, Position& position);
MovesList all_moves(Position& position);

using MoveListStack = std::array<MovesList, max_ply>;

struct ScoredMove {
    Move move;
    int score;
};

class ScoredMoveList {
public:
    static constexpr std::size_t capacity = 256;

    ScoredMoveList() noexcept = default;

    void push(ScoredMove move) noexcept {
        if (full()) { return; }

        std::size_t i = count_++;
        while (i > 0 && moves_[i - 1].score >= move.score) {
            moves_[i] = moves_[i - 1]; // shift the better-or-equal move right
            i--;
        }
        moves_[i] = move;
    }

    std::size_t size() const noexcept { return count_; }
    bool full() const noexcept { return count_ >= capacity; }
    bool empty() const noexcept { return count_ == 0; }
    void clear() noexcept { count_ = 0; }

    Move next_move() {
        assert(!exhausted());
        count_--;
        Move move = moves_[count_].move;
        return move;
    }
    bool exhausted() const noexcept { return count_ == 0; }

private:
    std::array<ScoredMove, capacity> moves_{};
    std::size_t count_ = 0;
};

void generate_scored_moves(ScoredMoveList& legal_moves, Position& position);

using ScoredMoveListStack = std::array<ScoredMoveList, max_ply>;
