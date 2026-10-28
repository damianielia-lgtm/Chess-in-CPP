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

class MovePicker {
public:
    static constexpr std::size_t capacity = 256;

    MovePicker(
        Position& position,
        std::array<ScoredMove, capacity>& move_list
    ) : position_(position), moves_(move_list) {}

    Move next_move();

private:
    enum class GenerationStage {
        NotStarted,
        Promotions,
        Captures,
        Quiets
    };

    Position& position_;
    std::array<ScoredMove, capacity>& moves_;
    std::size_t count_ = 0;
    GenerationStage stage_ = GenerationStage::NotStarted;
    bool first_move_ = true;
    bool sorted_ = false;
};

using ScoredMoveListStack = std::array<std::array<ScoredMove, MovePicker::capacity>, max_ply>;
