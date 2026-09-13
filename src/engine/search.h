#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../core/position.h"
#include "../core/move.h"

std::int16_t minimax(Position& position, std::uint8_t depth);

struct SearchedMove {
    std::optional<Move> move;
    std::int16_t eval;
};

struct NullObserver {
    static constexpr bool track_stats = false;

    void on_node() noexcept {}
    void on_leaf() noexcept {}
};

struct BasicStatsObserver {
    static constexpr bool track_stats = true;

    std::uint64_t nodes = 0;
    std::uint64_t leaf_nodes = 0;

    void on_node() noexcept { nodes++; }
    void on_leaf() noexcept { leaf_nodes++; }
};

template <typename Observer>
SearchedMove pick_best_move(Position& position, std::uint8_t depth, Observer& observer);

SearchedMove pick_best_move(Position& position, std::uint8_t depth);
std::vector<SearchedMove> rank_moves(Position& position, std::uint8_t depth);
