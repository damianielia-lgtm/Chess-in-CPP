#pragma once

#include <array>
#include <cassert>

#include "move.h"

class MovesList {
public:
    static constexpr std::size_t capacity = 256;

    MovesList() noexcept = default;

    void push(Move move) noexcept {
        assert(!full());
        moves_[count_++] = move;
    }

    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] bool full() const noexcept { return count_ == capacity; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] void clear() noexcept { count_ = 0; }

    using iterator = std::array<Move, capacity>::iterator;
    using const_iterator = std::array<Move, capacity>::const_iterator;

    iterator begin() { return moves_.begin(); }
    iterator end() { return moves_.begin() + count_; }

    const_iterator begin() const { return moves_.begin(); }
    const_iterator end() const { return moves_.begin() + count_; }

private:
    std::array<Move, capacity> moves_{};
    std::size_t count_ = 0;
};

constexpr std::size_t max_ply = 16;
using MoveListStack = std::array<MovesList, max_ply>;